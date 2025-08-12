import urequests
import network
import uasyncio

def connect_to_wifi(wifi_ssid, wifi_pass):
    sta_if = network.WLAN(network.STA_IF)
    if not sta_if.isconnected():
        print('connecting to network...')
        sta_if.active(True)
        sta_if.connect(wifi_ssid, wifi_pass)
        while not sta_if.isconnected():
            pass

def get_local_ip():
    sta_if = network.WLAN(network.STA_IF)
    if (sta_if.ifconfig() == ('0.0.0.0', '0.0.0.0', '0.0.0.0', '0.0.0.0')):
        return False
    else:
        return sta_if.ifconfig()[0]