#include <iostream>
#include <vector>

struct Buffer
{
	typedef std::vector<int16_t>::iterator Iterator;
    private:
        std::vector<int16_t> buffer;
        size_t offset;

    public:
        Buffer() : offset(0) {}

        void resize(size_t size)
        {
           buffer.resize(size);
        }

        void show10()
        {
            for (size_t i = 0; i < 10; i++)
                std::cout << buffer[i] << " | ";
        }

        void put(Iterator begin, Iterator end)
        {
	        size_t elems = end - begin;
	        if (elems > buffer.size())
	        	throw std::out_of_range("Size of the buffer is smaller than the amount of elements");

	        size_t free_elems = buffer.size() - offset;
	        if (elems > free_elems)
	        {
	        	size_t to_remove = elems - free_elems;
	        	std::copy(buffer.begin() + to_remove, buffer.end() - free_elems,
	        	          buffer.begin());
	        	offset -= to_remove;
	        }
	        std::copy(begin, end, buffer.begin() + offset);
	        offset += elems;
        }

        size_t get(size_t elems, std::vector<int16_t> &dest)
        {
	        if (offset == 0)
	        	return 0;

	        // If the amount of requested samples is bigger than available, return only
	        // available.
	        if (elems > offset)
	        	elems = offset;

	        if (elems >= dest.size())
	        {
	        	// If dest is smaller than the available amount of samples, discard the
	        	// ones that come first.
	        	size_t elems_lost = elems - dest.size();
	        	std::copy(buffer.begin() + elems_lost, buffer.begin() + elems, dest.begin());
	        }
	        else
	        {
	        	// Copy samples to the destination buffer.
	        	std::copy(dest.begin() + elems, dest.end(), dest.begin());
	        	std::copy(buffer.begin(), buffer.begin() + elems, dest.end() - elems);
	        }

	        // Remove them from the internal buffer.
	        std::copy(buffer.begin() + elems, buffer.begin() + offset, buffer.begin());
	        offset -= elems;

	        return elems;
        }

        size_t size() const
        {
        	return offset;
        }
};
