import network_manager
import ujson

try:
    network_data_file = open("network.json", "r")
    network_data = ujson.load(network_data_file)
    network_data_file.close()
    network_manager.connect_to_wifi(network_data['SSID'], network_data['PASS'])
except OSError:
    print("There was a problem loading the network config file")