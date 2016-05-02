#ifndef IMAGES_HPP_
#define IMAGES_HPP_

#include"spi_programmer.hpp"

// THIS IS NOT USED YET - it is meant for loading static images onto target
// - left here until I get time to add images (if that is a good idea at all)

namespace images
{
	// Use Serial to present choice of images, return selected one
	// in program memory. Ensure Serial is initialised with Serial.begin()
	// prior to calling this function.
	// Note returned image is in program memory.
	const spi_programmer::image_pgm_t& user_selected_image();

	// Return the device struct in program memory for given signature.
	// Return nullptr if device not found.
	const spi_programmer::device_pgm_t* device_for_signature(uint32_t);
}

#endif
