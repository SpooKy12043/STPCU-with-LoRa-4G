# STPCU-with-LoRa-4G


## Overview

This repository contains the source codes, technical considerations, and implementation details for the development of the telematic system and the intelligent module prototypes designed for the Urban Public Collective Transport System (STPCU) of the city of Tunja, Colombia.

The project integrates Internet of Things technologies to enhance operational efficiency, monitoring, and data-driven decision-making within public transport services.  
It makes use of LoRa, Wi-Fi, and 4G/LTE communication technologies to ensure reliable Edge-to-Cloud data transmission in urban environments.

## Project Scope

- Development of a telematic architecture based on IoT principles for STPCU.
- Design and prototyping of intelligent modules for vehicles and bus stops.
- Deployment of Edge devices capable of environmental monitoring and communication through LoRa and LTE networks.
- Integration with Cloud platforms for remote monitoring, data storage, and system analytics.

## Technologies Used

- LoRa for low-power, long-distance wireless communication.
- Wi-Fi for local high-speed data transfer as test.
- 4G/LTE for wide-area internet connectivity.

## Others (Devices, Sensors and Network Server)

- ESP8266/ESP32 microcontrollers** for module development.
- **Environmental sensors** (e.g., DHT11, PMS7003) for real-time data collection.
- **The Things Network (TTN)** for LoRaWAN integration.

Regarding LoRa technology, the codes used for uploading the data frame to TTN are based on the control codes provided by the **RadioLib** library. Modifications were made to the transmission and frame handling to suit the proposed scenario.

