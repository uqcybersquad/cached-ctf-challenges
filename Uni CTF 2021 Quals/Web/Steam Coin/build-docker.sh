#!/bin/bash
docker rm -f web_steamcoin
docker build -t web_steamcoin . 
docker run --name=web_steamcoin --rm -p1337:80 -it web_steamcoin
