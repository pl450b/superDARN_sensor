#!/bin/bash

SESSION="superDARN"
SESSIONEXISTS=$(tmux list-sessions | grep $SESSION)

# Only create tmux session if it doesn't already exist
if [ "$SESSIONEXISTS" = "" ]
then
  tmux new-session -d -s $SESSION
  
  # Setup sensor unit environment
  tmux rename-window -t 0 'Sensor_Unit'
  tmux send-keys -t 'Sensor_Unit' 'cd ./sensor_unit/main && nvim main.cpp' C-m
  tmux split-window -h
  tmux send-keys -t 'Sensor_Unit' 'source ~/esp/esp-idf/export.sh' C-m
  tmux send-keys -t 'Sensor_Unit' 'cd ./sensor_unit && idf.py build' C-m

  # Setup sensor hub environment
  tmux new-window -t $SESSION:1 -n 'Sensor_Hub'
  tmux send-keys -t 'Sensor_Hub' 'cd ./sensor_hub/main && nvim main.cpp' C-m
  tmux split-window -h
  tmux send-keys -t 'Sensor_Hub' 'source ~/esp/esp-idf/export.sh' C-m
  tmux send-keys -t 'Sensor_Hub' 'cd ./sensor_hub && idf.py build' C-m
  
  # Setup flashing/monitor window
  tmux new-window -t $SESSION:2 -n 'FnM'
  tmux send-keys -t 'FnM' 'cd ./sensor_hub && source ~/esp/esp-idf/export.sh' C-m
  tmux split-window -h
  tmux send-keys -t 'FnM' 'cd ./sensor_unit && source ~/esp/esp-idf/export.sh' C-m

  # Setup host computer program environment
  tmux new-window -t $SESSION:3 -n 'Host_Comp'
  tmux send-keys -t 'Host_Comp' 'cd ./host_computer && nvim main.cpp' C-m
  tmux split-window -h
fi 

# Attach Session, on the Main window
tmux attach-session -t $SESSION:1
