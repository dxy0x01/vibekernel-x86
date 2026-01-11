void dummy_test_function() {
}

void main() {
    // Pointer to video memory
    char* video_memory = (char*)0xb8000;
    
    // Write "X" at top left
    *video_memory = 'X';
    *(video_memory + 1) = 0x0f; // White on black
}
