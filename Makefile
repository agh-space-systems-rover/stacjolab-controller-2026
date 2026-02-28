b: 
	@echo "Building project..."
	@idf.py build

f: build
	@echo "Flashing project to device..."
	@idf.py flash

m:
	@echo "Starting serial monitor..."
	@idf.py monitor