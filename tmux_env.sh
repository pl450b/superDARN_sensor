#!/bin/bash

MY_IDF_PATH="~/esp/esp-idf"
SESSION="superDARN"
SESSIONEXISTS=$(tmux list-sessions | grep $SESSION)

# Only create tmux session if it doesn't already exist
if [ "$SESSIONEXISTS" = "" ]
then
  tmux new-session -d -s $SESSION
  
  # Setup sensor unit environment
  tmux rename-window -t 0 'Sensor_Unit'
  tmux send-keys -t 'Sensor_Unit' 'cd ./sensor_unit/main && nvim sensor_unit.c' C-m
  tmux split-window -h
  tmux send-keys -t 'Sensor_Unit' 'source $MY_IDF_PATH/export.sh' C-m
  tmux send-keys -t 'Sensor_Unit' './sensor_unit/main && ls' C-m

  # Setup sensor hub environment
  tmux new-window -t $SESSION:1 -n 'Sensor_Hub'
  tmux send-keys -t 'Sensor_Hub' 'cd ./sensor_hub/main && nvim sensor_hub.c' C-m
  tmux split-window -h
  tmux send-keys -t 'Sensor_Hub' 'source $MY_IDF_PATH/export.sh' C-m
  tmux send-keys -t 'Sensor_Hub' './sensor_hub/main && ls' C-m
  
  # Setup host computer program environment
  tmux new-window -t $SESSION:2 -n 'Host_Comp'
  tmux send-keys -t 'Host_Comp' 'cd ./host_computer && nvim main.cpp' C-m
  tmux split-window -h
fi 

# Attach Session, on the Main window
tmux attach-session -t $SESSION:1
