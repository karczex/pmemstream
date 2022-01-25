#! /usr/bin/awk -f
BEGIN {
	print "path, stream_size, block_size, region_size, element_count, element size, null region runtime, iterations, mean, max, min,"
}

/Pmemstream Benchmark*/ {
	split($0, line, ",");
	for (i in line) {
		split(line[i], var, ":")
		if (var[2]) {
			printf(var[2] ",")
		}
	}
}

/\smean:/ {split($0, line, ":");
	 printf(line[2] ",");
}

/\smax:/ {split($0, line, ":");
	 printf(line[2] ",");
}

/\smin:/ {split($0, line, ":");
	 printf(line[2] ",");
}
/####/ { printf("\n")}
