# import microserver
from umqtt.robust import MQTTClient
import ubinascii
import machine
import time
import ujson
import urequests

import random
import tpm
import array
import struct

import gc

localTree = tpm.TPM(1,[3],[3],3,1,0,0)

client_id = ubinascii.hexlify(machine.unique_id())
client_token = ""
topic_sub = b'tpm-notifications'
topic_pub = b'notification'
initMsg = {
  'deviceId': client_id,
  'iteration_count': 0,
}

http_server_ip=''
http_server_port=''
mqtt_broker_ip=''
mqtt_broker_port=''

INIT_ENDPOINT = ''

try:
    network_data_file = open("network.json", "r")
    network_data = ujson.load(network_data_file)
    network_data_file.close()
    print("Network data loaded.")
    http_server_ip = network_data['HTTP_SERVER_IP']
    http_server_port = network_data['HTTP_SERVER_PORT']
    mqtt_broker_ip = network_data['MQTT_BROKER_IP']
    mqtt_broker_port = network_data['MQTT_BROKER_PORT']
    INIT_ENDPOINT = f"http://{http_server_ip}:{http_server_port}/new-tpm"
except:
    print("Error loading server settings.")



MQTT_SUB_TOPICS = {
    'INIT': f'{client_token}/init',
    'STIMULATE': f'{client_token}/stimulate',
    'SAVE': f'{client_token}/save',
}

MQTT_PUB_TOPICS = {
    'INIT_REQUEST': 'tpm-notifications/init-request',
    'INITIAL_UPDATE': 'tpm-notifications/initial-update',
    'WEIGHT_UPDATE': 'tpm-notifications/weight-update', 
    'STIMULUS_REQUEST': 'tpm-notifications/stimulus-request'
}

def update_topics():
   global MQTT_SUB_TOPICS
   MQTT_SUB_TOPICS = {
    'INIT': f'{client_token}/init',
    'STIMULATE': f'{client_token}/stimulate',
    'SAVE': f'{client_token}/save',
}

def parse(topic,data):
    # print(data)
    if(topic == MQTT_SUB_TOPICS['STIMULATE']):
        parse_new_stimulus(data)
        return
    # if(topic == MQTT_SUB_TOPICS['SAVE']):
    #     save_current_tpm(data)
    #     return

    print(f"No command found for topic: {topic}")

def parse_init_request(data):
  global localTree
  global client_token
  try:
    if('deviceToken' in data):
        initSettings = data['settings']
        client_token = data['deviceToken']
        update_topics()

        print(initSettings)
      
        if(client_token != ""):
          rules = {
            "HEBBIAN": 0,
          }
          scenarios = {
            "NO_OVERLAP": 0,
            "PARTIAL_OVERLAP": 1,
            "FULL_OVERLAP": 2,
          }
          localTree = tpm.TPM(initSettings["H"], initSettings["K"],initSettings["N"],initSettings["L"], initSettings["M"], rules[initSettings["LearnRule"]], scenarios[initSettings["LinkType"]])
          print("[STATUS]: Subscribing to TPM channels...")
          for topic in MQTT_SUB_TOPICS.values():
            client.subscribe(topic)
            print(f"\t- Subscribed to: {topic}")
        else:
          print("No client token was found.")

        initialUpdate = {
          'deviceToken': client_token,
          'iterationCount': 0,
          'weights': [struct.unpack('b', bytes([byte]))[0] for byte in localTree.get_weights()],
          'output': 0,
          'learnRule': initSettings["LearnRule"],
        }
        return initialUpdate

  except Exception as e:
    print(f"An error ocurred while parsing new_tpm_request: {e}")

def parse_new_stimulus(data):
  try:
    if ('FlatInput' in data):
      inputBuf = struct.pack(f'{len(data['FlatInput'])}b',*data['FlatInput'])
      localResult = localTree.calculate(inputBuf)
      if(localResult == data['Output']):
        print(f'local result: {localResult} | remote result: {data["Output"]} | output: {localResult==data["Output"]}')
        localTree.learn(inputBuf, localResult)
        outputMsg = {
          'deviceToken': client_token,
          'weights': [struct.unpack('b', bytes([byte]))[0] for byte in localTree.get_weights()],
          'output': localResult
        }
        # print(ujson.dumps(outputMsg))
        print("Free Memory:", gc.mem_free())
        client.publish(MQTT_PUB_TOPICS['WEIGHT_UPDATE'],ujson.dumps(outputMsg))
      else:
        outputMsg = {
          'deviceToken': client_token,
        }
        client.publish(MQTT_PUB_TOPICS['STIMULUS_REQUEST'],ujson.dumps(outputMsg))


  except Exception as e:
    print(f"An error ocurred while parsing new_stimulus_request: {e}")

# def parse_learn_request(data):
#     if(data == "OK"):
#         localTree.learn(learnRule)
#         outputMsg = {
#           'deviceToken': client_token,
#           'weights': [struct.unpack('b', bytes([byte]))[0] for byte in localTree.get_weights()],
#         }
#         client.publish(MQTT_PUB_TOPICS['WEIGHT_UPDATE'],ujson.dumps(outputMsg))

# def save_current_tpm(data):
#   print("TPMs in sync, saving to data.json")
#   syncedTpm = {'token_uid': client_token,'K': localTree.K,'N': localTree.N, 'L': localTree.L, 'weights':localTree.getWeights().tolist()}
#   with open('data.json', 'w') as fp:
#     ujson.dump(syncedTpm, fp)

#   print("TPM saved.")

def sub_cb(topic, msg):
  # print("[STATUS]: New message arrived.")
  if(topic.decode() in MQTT_SUB_TOPICS.values()):   
    try:
      loadedJson = ujson.loads(msg)
      response = ""
      if("data" in loadedJson):
        parse(topic.decode(),loadedJson['data'])
      else:
        parse(topic.decode(), loadedJson)
    except Exception as e:
      print(f"An error ocurred while parsing control msg: {e}")

def post_new_tpm(init_json):
    try:
        res = urequests.post(INIT_ENDPOINT, headers = {'Content-Type': 'application/json'}, data=init_json)
        data = ujson.loads(res.content)
        return data
    except ValueError as e:
        print(e)


def create_packed_buffer(n):
    # Generate a list of n random elements where each element is -1 or 1
    random_list = [random.choice([-1, 1]) for _ in range(n)]
    
    # Use struct.pack to pack the list as a sequence of signed chars ('b')
    # The '*' unpacks the list into individual arguments for struct.pack
    buf = struct.pack(f'{n}b', *random_list)
    
    return buf

if(INIT_ENDPOINT != ''):
  client = MQTTClient(client_id, mqtt_broker_ip,mqtt_broker_port,keepalive=3600)
  client.set_callback(sub_cb)
  client.connect()
  response = post_new_tpm(ujson.dumps(initMsg))
  print(response)
  initial_update = parse_init_request(response)
  print(initial_update)
  print("[STATUS]: Publishing initialization request...")
  client.publish(MQTT_PUB_TOPICS['STIMULUS_REQUEST'],ujson.dumps(initial_update))

  connection_errors = 0
  iterations = 0
  while True:
    try:
      client.check_msg()
      connection_errors = 0
    except OSError as e:
      print('[ERROR]: Failed to connect to MQTT broker.')
      print('\t- Resetting in 5 seconds...')
      time.sleep(5)
      machine.reset()