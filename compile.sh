BASE_DIR=$(pwd)
. $BASE_DIR/esp-idf/export.sh
cd $BASE_DIR/micropython/
make -C mpy-cross
cd ports/esp32
make submodules
make USER_C_MODULES=$BASE_DIR/user_c_modules

# Check if the final make command failed
if [ $? -ne 0 ]; then
    echo "Error: make command failed!"
    exit 1
fi

python -m esptool --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 4MB --flash_freq 40m 0x1000 build-ESP32_GENERIC/bootloader/bootloader.bin 0x8000 build-ESP32_GENERIC/partition_table/partition-table.bin 0x10000 build-ESP32_GENERIC/micropython.bin