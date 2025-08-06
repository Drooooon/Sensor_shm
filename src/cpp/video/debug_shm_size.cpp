#include "image_shm_manager.h"
#include <iostream>

int main() {
  std::cout << "=== Shared Memory Size Debug ===" << std::endl;
  std::cout << "NUM_BUFFERS: " << NUM_BUFFERS << std::endl;
  std::cout << "sizeof(ShmBufferControl): " << sizeof(ShmBufferControl) << " bytes" << std::endl;
  
  size_t buffer_size = 5 * 1024 * 1024; // 5MB
  size_t total_size = 20 * 1024 * 1024; // 20MB
  
  std::cout << "Buffer size: " << buffer_size << " bytes (" << buffer_size / (1024*1024) << " MB)" << std::endl;
  std::cout << "Total size: " << total_size << " bytes (" << total_size / (1024*1024) << " MB)" << std::endl;
  
  size_t required_size = sizeof(ShmBufferControl) + NUM_BUFFERS * buffer_size;
  std::cout << "Required size: " << required_size << " bytes (" << required_size / (1024*1024) << " MB)" << std::endl;
  
  if (total_size >= required_size) {
    std::cout << "✓ Memory size validation should PASS" << std::endl;
  } else {
    std::cout << "✗ Memory size validation will FAIL" << std::endl;
    std::cout << "Need additional: " << (required_size - total_size) << " bytes" << std::endl;
  }
  
  std::cout << "\nTesting actual shared memory creation..." << std::endl;
  ImageShmManager test_shm("debug_test_shm");
  ShmStatus result = test_shm.create_and_init(total_size, buffer_size);
  
  if (result == ShmStatus::Success) {
    std::cout << "✓ Shared memory creation SUCCESS" << std::endl;
    test_shm.unmap_and_close();
    test_shm.unlink_shm();
  } else {
    std::cout << "✗ Shared memory creation FAILED: " << static_cast<int>(result) << std::endl;
  }
  
  return 0;
}
