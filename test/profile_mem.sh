# Run qemu with -d mmu
# input MMU log trace
QEMU_DIR=${TOP}/qemu/
MEMTRACE=$1
MEMTRACE_TMP=$MEMTRACE.tmp
MEMTRACE_ADDR=$MEMTRACE.addr
MEMTRACE_SORTED_ADDR=$MEMTRACE.sorted
${QEMU_DIR}/scripts/simpletrace.py ${QEMU_DIR}/build/trace/trace-events-all ${MEMTRACE} > ${MEMTRACE_TMP}
cat ${MEMTRACE_TMP} |  cut -d" " -f 4,6,8,5 > ${MEMTRACE_ADDR}
cat ${MEMTRACE_ADDR} | sort --unique  > ${MEMTRACE_SORTED_ADDR}
cat ${MEMTRACE_SORTED_ADDR} | cut -d " " -f 2 > ${MEMTRACE}.regionid
regionids=$(cat ${MEMTRACE}.regionid)
for i in `seq 0 4`
do 
for rid in $regionids
do
HART_TRACE=${MEMTRACE}.${rid}.$i
cat ${MEMTRACE_ADDR} |grep "cpu_index=0x$i" |grep ${rid} > ${HART_TRACE}
awk '($1 != prev) { print $0; prev=$1 }' ${HART_TRACE} > ${HART_TRACE}.short
cat ${HART_TRACE} | sort --unique  > ${HART_TRACE}.short
done
done