#!/bin/bash


docker build -t hackrva/badgesim2024 -f ./web/deployments/wasmsim.dockerfile .

# run:
# docker run -p 8080:8080 hackrva/badgesim2024

# deploy:
# docker push hackrva/badgesim2024
