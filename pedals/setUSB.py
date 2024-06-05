Import("env")

# Function to update USB configuration
def update_usb_configuration(pid):
    print("Updating USB configuration...")

    board_config = env.BoardConfig()
    board_config.update("build.usb_product", "Arduino Pedals")
    board_config.update("build.usb_manufacturer", "Jamuino")
    board_config.update("build.hwids", [["0x2345", pid]])

    print(f"USB Product: {board_config.get('build.usb_product')}")
    print(f"USB Manufacturer: {board_config.get('build.usb_manufacturer')}")
    print(f"USB HWIDs: {board_config.get('build.hwids')}")

new_pid = "0x8041"
update_usb_configuration(new_pid)
