 gcovr -r /home/fed/Documents/clion-workspace/qt-try --exclude-unreachable-branches 2>&1 | grep "^src/" | awk '{
     split($4,pct,"%"); lines+=$2; exec+=$3; if($2>0) count++                                                              
   } END {
     printf "Total files: %d\n", count
     printf "Total lines:  %d\n", lines
     printf "Executed:     %d\n", exec
     printf "Line coverage: %.1f%%\n", (exec/lines)*100
   }'