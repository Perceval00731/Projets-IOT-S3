#!/usr/bin/env python3
# -*- coding: utf8 -*-

# Le device

import paho.mqtt.client as mqtt
import json
import logging

# Configuration
mqttServer = "chirpstack.iut-blagnac.fr"
appID = "6"
devEUI = "0c7e450102030110"  # à adapter
topic_subscribe = f"application/{appID}/device/{devEUI}/event/up"
topic_publish = f"application/{appID}/device/{devEUI}/command/down"
blue = "Ymx1ZQ=="
orange = "b3Jhbmdl"

logging.basicConfig(level=logging.INFO)

# Callback de réception des messages
def get_data(mqttc, obj, msg):
    try:
        # Désérialisation du message
        jsonMsg = json.loads(msg.payload)
        
        # Extraction des info
        rxInfo = jsonMsg.get('rxInfo', [])
        object_data = jsonMsg.get('object', {})
        
        # Affichage des informations
        for info in rxInfo:
            time = info.get('time')
            if time:
                print(f"Time: {time}")
        
        # Désérialiser
        mydata_str = object_data.get('mydata')
        if mydata_str:
            mydata = json.loads(mydata_str)
            co2_concentration = mydata.get('CO2')
            
            if co2_concentration is not None:
                print(f"CO2 Concentration: {co2_concentration} ppm")
            else:
                logging.error("Concentration de CO2 non trouvée dans le message")
        else:
            logging.error("Champ 'mydata' non trouvé dans le message")
        
    except (json.JSONDecodeError, KeyError) as e:
        logging.error("Erreur dans les données reçues : %s", e)

# Fonction d'envoi de commande
def send(val):
    # TODO
    logging.info(f"Commande envoyée : {val}")

# Connexion et souscription
mqttc = mqtt.Client()
mqttc.connect(mqttServer, port=1883, keepalive=60)
mqttc.on_message = get_data
mqttc.subscribe(topic_subscribe, qos=0)
mqttc.loop_forever()