# Finds the statement that changes frames_sent without a call to send_frame.
set pagination off
break set_name
run
delete
watch link_state.frames_sent
continue
backtrace 3
