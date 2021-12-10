gcov $(find . -maxdepth 10 -type f -name "*.gcno") |grep Lines| grep of|cut -f 4 -d" " > lines.txt
awk '{for(i=1;i<=NF;++i){c+=$i};print $0, "Sum:", c}' < lines.txt
