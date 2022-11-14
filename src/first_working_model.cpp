// #include <iostream>
#include <fcntl.h>
// #include <sys/types.h>
#include <unistd.h>
#include <vector>
// #include <cmath>
// #include <signal.h>
// #include <ncurses.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "buffer.cpp"
#include "shaders/shader.h"


std::string SOURCE_PATH = "/tmp/mpd.fifo";
int m_source_fd = -1;
std::vector<int16_t> m_incoming_samples;
Buffer m_buffered_samples;
std::vector<int16_t> m_rendered_samples;
const size_t columns = 90;
const size_t fps = 24;
size_t m_sample_consumption_rate = 5;
size_t m_sample_consumption_rate_up_ctr = 0;
size_t m_sample_consumption_rate_dn_ctr = 0;
// Is output stereo
bool isStereo = 1;

std::vector<float> gpuValues;
unsigned int VBO;
unsigned int VAO;


// File handling
void openFile()
{
	if (m_source_fd >= 0)
		return;
	m_source_fd = open(SOURCE_PATH.c_str(), O_RDONLY | O_NONBLOCK);
	if (m_source_fd < 0)
    {
        std::cout << "Can't open file: " << SOURCE_PATH << "\n";
    }
}

void closeFile()
{
	if (m_source_fd >= 0)
		close(m_source_fd);
	m_source_fd = -1;
}

// Initialize
void init()
{
    size_t slow = 1;
	ssize_t rendered_samples = ceil(44100.0 / fps / columns);
    rendered_samples *= columns;
    rendered_samples *= slow;
    if (isStereo)
        rendered_samples *= 2;

	// Keep 500ms worth of samples in the incoming buffer.
	size_t buffered_samples = 44100.0 / 2;
	if (isStereo)
		buffered_samples *= 2;
	m_rendered_samples.resize(rendered_samples);
	m_incoming_samples.resize(buffered_samples);
	m_buffered_samples.resize(buffered_samples);

    // OpenGL
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    if (isStereo)
    {
        gpuValues.resize(4*columns + 4);
        gpuValues[0] = gpuValues[1] = gpuValues[4*columns + 3] = -1.0f; gpuValues[4*columns + 2] = 1.0f;
    }
    else
    {
        gpuValues.resize(2*columns + 4);
        gpuValues[0] = gpuValues[1] = gpuValues[2*columns + 3] = -1.0f; gpuValues[2*columns + 2] = 1.0f;
    }

    // Initialize x coords
    float x = -1.0f;
    if (isStereo)
    {
        for (size_t i = 0; i < 2*columns; ++i, x += 2.0f/(2*columns-1))
            gpuValues[2*i + 2] = x;
    }

    if (isStereo)
    {
        glBufferData(GL_ARRAY_BUFFER, 4*(columns + 1)*sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    }
    glEnableVertexAttribArray(0);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void clearData()
{
	std::fill(m_rendered_samples.begin(), m_rendered_samples.end(), 0);

	// Discard any lingering data from the data source.
	if (m_source_fd >= 0)
	{
		ssize_t bytes_read;
		do
			bytes_read = read(m_source_fd, m_incoming_samples.data(), sizeof(int16_t) * m_incoming_samples.size());
		while (bytes_read > 0);
	}
}

void update()
{
	if (m_source_fd < 0)
		return;

	ssize_t bytes_read = read(m_source_fd, m_incoming_samples.data(), sizeof(int16_t) * m_incoming_samples.size());
    if (!bytes_read)
        std::cout << "Read error" << std::endl;
    // std::cout << "bytes read: " << bytes_read << " >> ";

    if (bytes_read > 0)
	{
		const auto begin = m_incoming_samples.begin();
		const auto end = m_incoming_samples.begin() + bytes_read/sizeof(int16_t);

		m_buffered_samples.put(begin, end);
        // std::cout << "Putted. ";
    }

	size_t requested_samples = 44100.0 / fps * pow(1.1, m_sample_consumption_rate);
	if (isStereo)
		requested_samples *= 2;

    // Receive buffered samples
	size_t new_samples = m_buffered_samples.get(requested_samples, m_rendered_samples);
	if (new_samples == 0)
		return;

	if (m_buffered_samples.size() > 0)
	{
		if (++m_sample_consumption_rate_up_ctr > 8)
		{
			m_sample_consumption_rate_up_ctr = 0;
			++m_sample_consumption_rate;
		}
	}
	else if (m_sample_consumption_rate > 0)
	{
		if (++m_sample_consumption_rate_dn_ctr > 4)
		{
			m_sample_consumption_rate_dn_ctr = 0;
			--m_sample_consumption_rate;
		}
		m_sample_consumption_rate_up_ctr = 0;
	}

    size_t d_count = new_samples;
    size_t samples_per_column = d_count/columns;
    if (isStereo)
        samples_per_column /= 2;

	if (isStereo)
	{
		int16_t buf_left[d_count/2], buf_right[d_count/2];
		for (size_t i = 0, j = 0; i < d_count; i += 2, ++j)
		{
			buf_left[j] = m_rendered_samples[i];
			buf_right[j] = m_rendered_samples[i+1];
		}

        for (size_t i = 0; i < columns; ++i)
        {
            int32_t sum = 0;
            for (size_t j = 0; j < samples_per_column; ++j)
            {
                sum += buf_left[i*samples_per_column + j];
            }
            sum = std::abs(sum);
            gpuValues[2*i + 3] = sum / 65536.0f / samples_per_column;
        }

        for (size_t i = 0; i < columns; ++i)
        {
            int32_t sum = 0;
            for (size_t j = 0; j < samples_per_column; ++j)
            {
                sum += buf_right[i*samples_per_column + j];
            }
            sum = std::abs(sum);
            gpuValues[4*columns + 1 - 2*i] = sum / 65536.0f / samples_per_column;
        }
	}
	else
	{
        printf("lmao no");
	}

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*gpuValues.size(), &gpuValues[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}





/**************** OpenGL ****************/

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}  


void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}




int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  
    // Create window and error handling
    GLFWwindow* window = glfwCreateWindow(800, 600, "Visualizer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Do stuff in specified window
    glfwMakeContextCurrent(window);

    // load glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    


    Shader myShader("../src/shaders/vertex.glsl", "../src/shaders/fragment.glsl");

    openFile();
    init();

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  

    // struct timespec tim, tim2;
    // tim.tv_sec = 0;
    // tim.tv_nsec = 41670000L;
    //
    // signal(SIGINT, signal_callback_handler);
    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClearColor(25.0f/255.0f, 23.0f/255.0f, 36.0f/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        myShader.use();
        update();
        for (size_t i = 0; i < 4*columns + 4; ++i)
            std::cout << gpuValues[i] << " | ";
        std::cout << std::endl;
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINE_LOOP, 0, 2*columns+2);
        glfwPollEvents();    
        glfwSwapBuffers(window);
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    clearData();
    closeFile();
    return 0;
}
