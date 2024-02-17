import ctypes

# Load the C functions directly
example_lib = ctypes.CDLL(None)

# Call the 'hello_from_c' function
example_lib.hello_from_c()

# Call the 'add' function
result = example_lib.add(3, 4)
print("Result of add function:", result)
