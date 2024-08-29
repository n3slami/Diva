

# Parse Rosetta
filename="dst.txt"
echo "******************* Rosetta ******************* "
cat $filename | grep "Write numEntries elements took"
read_line=$(grep -n "Starting range query" $filename | cut -d: -f1)
echo "Reading dst starts at line $read_line"
cat $filename | grep "Read numQueries elements took"

tail -n +$read_line $filename | grep "RangeMayExist" | cut -f2 > result.txt
total_filter_probe_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
#echo "Total filter probe time (new): $total_filter_probe_time ns" 
tail -n +$read_line $filename | grep "GetFilterBitsReader" | cut -f2 > result.txt
filter_bits_reader_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "StaticCast" | cut -f2 > result.txt
static_cast_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "deserialize" | cut -f2 > result.txt
deserialize_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
#echo "deserialize_time: $deserialize_time"

tail -n +$read_line $filename | grep "SeekImpl" | cut -c10-30 > result.txt
total_block_seek_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "####1" | cut -c8-30 > result.txt
filter_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "####2" | cut -c8-30 > result.txt
index_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "####3" | cut -c8-30 > result.txt
data_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
echo "Filter seek time (ns): $total_filter_probe_time (probe) + $filter_bits_reader_time (deserialize) + $static_cast_time (static cast) " #+ $((filter_part-total_filter_probe_time-filter_bits_reader_time-static_cast_time)) (other)" 
#echo "Block seek time: $total_block_seek_time ns = $filter_part + $index_part + $data_part" 

cat result.txt | grep "uses" | cut -d: -f2
cat $filename | grep "uses:"
cat $filename | grep "False positive"

# PERF context processing
echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename >  result.txt
db_seek=$(grep "rocksdb.db.seek.micros" $filename | cut -d: -f7)
db_seek=$((db_seek*1000)) # converts to ns
seek_internal_seek_time=$(cat result.txt | grep "seek_internal_seek_time"  | cut -d= -f2)
iter_next_cpu_nanos=$(cat result.txt | grep "iter_next_cpu_nanos" | cut -d= -f2)
iter_seek_cpu_nanos=$(cat result.txt | grep "iter_seek_cpu_nanos"  | cut -d= -f2)
block_read_count=$(cat result.txt | grep "block_read_count"  | cut -d= -f2 | head -n 1)
block_read_time=$(cat result.txt | grep "block_read_time"  | cut -d= -f2)
index_block_read_count=$(cat result.txt | grep "index_block_read_count"  | cut -d= -f2)
filter_block_read_count=$(cat result.txt | grep "filter_block_read_count"  | cut -d= -f2)
echo "db_seek: $db_seek seek_internal_seek_time: $seek_internal_seek_time"
echo "iter_next_cpu_nanos: $iter_next_cpu_nanos, iter_seek_cpu_nanos: $iter_seek_cpu_nanos"
echo "block_read_count: $block_read_count block_read_time:$block_read_time index_block_read_count: $index_block_read_count filter_block_read_count: $filter_block_read_count"
cat $filename | grep "rocksdb.range_filter.bpk_times_100" | cut -d: -f2
cat $filename | grep "RocksDB Estimated Table Readers Memory"
cat $filename | grep "rocksdb.compaction.times.micros"
cat $filename | grep "rocksdb.compaction.times.cpu_micros"
cat $filename | head -n $read_line | grep "creation" | cut -f2 > result.txt
creation_cost=$(awk '{ sum += $1 } END { print sum }' result.txt)
no_of_compaction=$(cat result.txt | wc -l)
echo "Filter creation time: $creation_cost"
cat $filename | grep "rocksdb.compact.read.bytes COUNT"
cat $filename | grep "rocksdb.compact.write.bytes COUNT"


#Parse SuRF
filename="surf.txt"
echo
echo "******************* SuRF ******************* "
cat $filename | grep "Write numEntries elements took"
read_line=$(grep -n "Starting range query" $filename | cut -d: -f1)
echo "Reading surf starts at line $read_line"
cat $filename | grep "Read numQueries elements took"

tail -n +$read_line $filename | grep "RangeMayExist" | cut -f2 > result.txt
total_filter_probe_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
#echo "Total filter probe time (new): $total_filter_probe_time ns" 
tail -n +$read_line $filename | grep "GetFilterBitsReader" | cut -f2 > result.txt
filter_bits_reader_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "StaticCast" | cut -f2 > result.txt
static_cast_time=$(awk '{ sum += $1 } END { print sum }' result.txt)

tail -n +$read_line $filename | grep "SeekImpl" | cut -c10-30 > result.txt
total_block_seek_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "####1" | cut -c8-30 > result.txt
filter_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "####2" | cut -c8-30 > result.txt
index_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
tail -n +$read_line $filename | grep "####3" | cut -c8-30 > result.txt
data_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
echo "Filter seek time (ns): $total_filter_probe_time (probe) + $filter_bits_reader_time (deserialize) + $static_cast_time (static cast)" # + $((filter_part-total_filter_probe_time-filter_bits_reader_time-static_cast_time)) (other)"
#echo "Block seek time: $total_block_seek_time ns = $filter_part + $index_part + $data_part"  

# grep "rocksdb.db.seek.micros" $filename
# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep "seek_internal_seek_time"
# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep -n "cpu"
cat $filename | grep "uses:"
cat $filename | grep "False positive"
# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep "block_read"

# PERF context processing
echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename >  result.txt
db_seek=$(grep "rocksdb.db.seek.micros" $filename | cut -d: -f7)
db_seek=$((db_seek*1000)) # converts to ns
seek_internal_seek_time=$(cat result.txt | grep "seek_internal_seek_time"  | cut -d= -f2)
iter_next_cpu_nanos=$(cat result.txt | grep "iter_next_cpu_nanos" | cut -d= -f2)
iter_seek_cpu_nanos=$(cat result.txt | grep "iter_seek_cpu_nanos"  | cut -d= -f2)
block_read_count=$(cat result.txt | grep "block_read_count"  | cut -d= -f2 | head -n 1)
block_read_time=$(cat result.txt | grep "block_read_time"  | cut -d= -f2)
index_block_read_count=$(cat result.txt | grep "index_block_read_count"  | cut -d= -f2)
filter_block_read_count=$(cat result.txt | grep "filter_block_read_count"  | cut -d= -f2)
echo "db_seek: $db_seek seek_internal_seek_time: $seek_internal_seek_time"
echo "iter_next_cpu_nanos: $iter_next_cpu_nanos, iter_seek_cpu_nanos: $iter_seek_cpu_nanos"
echo "block_read_count: $block_read_count block_read_time:$block_read_time index_block_read_count: $index_block_read_count filter_block_read_count: $filter_block_read_count"
cat $filename | grep "rocksdb.range_filter.bpk_times_100" | cut -d: -f2
cat $filename | grep "RocksDB Estimated Table Readers Memory"
cat $filename | grep "rocksdb.compaction.times.micros"
cat $filename | grep "rocksdb.compaction.times.cpu_micros"
cat $filename | head -n $read_line | grep "creation" | cut -f2 > result.txt
creation_cost=$(awk '{ sum += $1 } END { print sum }' result.txt)
no_of_compaction=$(cat result.txt | wc -l)
echo "Filter creation time: $creation_cost"
cat $filename | grep "rocksdb.compact.read.bytes COUNT"
cat $filename | grep "rocksdb.compact.write.bytes COUNT"


#Parse Prefix
echo
filename="prefix.txt"
if test -f "$filename"; then
    echo "$filename exists."

	echo "******************* Prefix ******************* "
	read_line=$(grep -n "Starting range query" $filename | cut -d: -f1)
	echo "Reading prefix starts at line $read_line"
	cat $filename | grep "Read numQueries elements took"
	tail -n +$read_line $filename | grep "RangeMayExist" | cut -c15-26 > result.txt
	total_filter_probe_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
	echo "Total filter probe time: $total_filter_probe_time ns"

	tail -n +$read_line $filename | grep "SeekImpl" | cut -c10-30 > result.txt
	total_block_seek_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
	tail -n +$read_line $filename | grep "####1" | cut -c8-30 > result.txt
	filter_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
	tail -n +$read_line $filename | grep "####2" | cut -c8-30 > result.txt
	index_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
	tail -n +$read_line $filename | grep "####3" | cut -c8-30 > result.txt
	data_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
	#echo "Block seek time: $total_block_seek_time ns = $filter_part + $index_part + $data_part"  

	# grep "rocksdb.db.seek.micros" $filename
	# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep "seek_internal_seek_time"
	# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep -n "cpu"
	cat $filename | grep "uses:"
	cat $filename | grep "False positive"
	# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep "block_read"

	# PERF context processing
	echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename >  result.txt
	db_seek=$(grep "rocksdb.db.seek.micros" $filename | cut -d: -f7)
	db_seek=$((db_seek*1000)) # converts to ns
	seek_internal_seek_time=$(cat result.txt | grep "seek_internal_seek_time"  | cut -d= -f2)
	iter_next_cpu_nanos=$(cat result.txt | grep "iter_next_cpu_nanos" | cut -d= -f2)
	iter_seek_cpu_nanos=$(cat result.txt | grep "iter_seek_cpu_nanos"  | cut -d= -f2)
	block_read_count=$(cat result.txt | grep "block_read_count"  | cut -d= -f2 | head -n 1)
	block_read_time=$(cat result.txt | grep "block_read_time"  | cut -d= -f2)
	index_block_read_count=$(cat result.txt | grep "index_block_read_count"  | cut -d= -f2)
	filter_block_read_count=$(cat result.txt | grep "filter_block_read_count"  | cut -d= -f2)
	echo "db_seek: $db_seek seek_internal_seek_time: $seek_internal_seek_time"
	echo "iter_next_cpu_nanos: $iter_next_cpu_nanos, iter_seek_cpu_nanos: $iter_seek_cpu_nanos"
	echo "block_read_count: $block_read_count block_read_time:$block_read_time index_block_read_count: $index_block_read_count filter_block_read_count: $filter_block_read_count"
	cat $filename | grep "rocksdb.range_filter.bpk_times_100" | cut -d: -f2
	cat $filename | grep "RocksDB Estimated Table Readers Memory"
fi

# Parse None
filename="none.txt"
if test -f "$filename"; then
    echo "$filename exists."
	echo
	echo "******************* None ******************* "
	cat $filename | grep "Write numEntries elements took"
	read_line=$(grep -n "Starting range query" $filename | cut -d: -f1)
	echo "Range query for none starts at line $read_line"
	cat $filename | grep "Read numQueries elements took"
	tail -n +$read_line $filename | grep "RangeMayExist" | cut -c15-26 > result.txt
	total_filter_probe_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
	echo "Total filter probe time: 0 ns" 

	tail -n +$read_line $filename | grep "SeekImpl" | cut -c10-30 > result.txt
	total_block_seek_time=$(awk '{ sum += $1 } END { print sum }' result.txt)
	tail -n +$read_line $filename | grep "####1" | cut -c8-30 > result.txt
	filter_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
	tail -n +$read_line $filename | grep "####2" | cut -c8-30 > result.txt
	index_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
	tail -n +$read_line $filename | grep "####3" | cut -c8-30 > result.txt
	data_part=$(awk '{ sum += $1 } END { print sum }' result.txt)
	#echo "Block seek time: $total_block_seek_time ns = $filter_part + $index_part + $data_part" 


	# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep -n "cpu"
	cat $filename | grep "uses:"
	cat $filename | grep "False positive"
	# echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename | grep "block_read"

	# PERF context processing
	echo $Variable | awk -F, '{ for (i = 1; i < NF; ++i ) print $i }' $filename >  result.txt
	db_seek=$(grep "rocksdb.db.seek.micros" $filename | cut -d: -f7)
	db_seek=$((db_seek*1000)) # converts to ns
	seek_internal_seek_time=$(cat result.txt | grep "seek_internal_seek_time"  | cut -d= -f2)
	iter_next_cpu_nanos=$(cat result.txt | grep "iter_next_cpu_nanos" | cut -d= -f2)
	iter_seek_cpu_nanos=$(cat result.txt | grep "iter_seek_cpu_nanos"  | cut -d= -f2)
	block_read_count=$(cat result.txt | grep "block_read_count"  | cut -d= -f2 | head -n 1)
	block_read_time=$(cat result.txt | grep "block_read_time"  | cut -d= -f2)
	index_block_read_count=$(cat result.txt | grep "index_block_read_count"  | cut -d= -f2)
	filter_block_read_count=$(cat result.txt | grep "filter_block_read_count"  | cut -d= -f2)
	echo "db_seek: $db_seek seek_internal_seek_time: $seek_internal_seek_time"
	echo "iter_next_cpu_nanos: $iter_next_cpu_nanos, iter_seek_cpu_nanos: $iter_seek_cpu_nanos"
	echo "block_read_count: $block_read_count block_read_time:$block_read_time index_block_read_count: $index_block_read_count filter_block_read_count: $filter_block_read_count"
	cat $filename | grep "rocksdb.range_filter.bpk_times_100" | cut -d: -f2
	cat $filename | grep "RocksDB Estimated Table Readers Memory"
	cat $filename | grep "rocksdb.compaction.times.micros"
	cat $filename | grep "rocksdb.compaction.times.cpu_micros"
	cat $filename | head -n $read_line | grep "creation" | cut -f2 > result.txt
	creation_cost=$(awk '{ sum += $1 } END { print sum }' result.txt)
	no_of_compaction=$(cat result.txt | wc -l)
	echo "Filter creation time: $creation_cost, no of compaction: $no_of_compaction"
	cat $filename | grep "rocksdb.compact.read.bytes COUNT"
	cat $filename | grep "rocksdb.compact.write.bytes COUNT"
fi

