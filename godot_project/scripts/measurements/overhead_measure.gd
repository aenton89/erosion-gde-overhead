extends RefCounted
class_name OverheadMeasure



var noop: ErosionNoOp



func _init() -> void:
	noop = ErosionNoOp.new()



func measure(profiler: Object, repeats: int, n: int, arr_sizes: Array, path: String, config: PipeErosionSettings, hm: Heightmap = null) -> void:
	profiler.begin_results_file(path, "variant,array_size,run,per_call_us")
	
	for run_idx in range(repeats):
		# baseline - pusta pętla bez wywołania natywnego
		var baseline_us: float = -1.0
		var t0: int = Time.get_ticks_usec()
		for i in n:
			pass
		baseline_us = float(Time.get_ticks_usec() - t0) / n
		
		# koszt przekroczenia granicy bez argumentów
		var noargs_us: float = -1.0
		t0 = Time.get_ticks_usec()
		for i in n:
			noop.noop()
		noargs_us = float(Time.get_ticks_usec() - t0) / n - baseline_us
		save_overhead_csv(path, "noop_noargs", 0, run_idx, noargs_us)
		
		var int_us: float = -1.0
		t0 = Time.get_ticks_usec()
		var integer: int = 7
		for i in n:
			noop.noop_int(integer)
		int_us = float(Time.get_ticks_usec() - t0) / n - baseline_us
		save_overhead_csv(path, "noop_int", 0, run_idx, int_us)
		
		var float_us: float = -1.0
		t0 = Time.get_ticks_usec()
		var flt: float = 0.1
		for i in n:
			noop.noop_float(flt)
		float_us = float(Time.get_ticks_usec() - t0) / n - baseline_us
		save_overhead_csv(path, "noop_float", 0, run_idx, float_us)
		
		# koszt z tablicą
		var array_us: float = -1.0
		for size in arr_sizes:
			var arr: PackedFloat32Array = PackedFloat32Array()
			arr.resize(size)
			t0 = Time.get_ticks_usec()
			for i in n:
				noop.noop_array(arr)
			array_us = float(Time.get_ticks_usec() - t0) / n - baseline_us
			save_overhead_csv(path, "noop_array", size, run_idx, array_us)
		
		# koszt mockupu - same dane erozji PipeErosion
		var mockup_us: float = -1.0
		if hm:
			var num: int = 2
			t0 = Time.get_ticks_usec()
			for i in n:
				noop.noop_mockup(hm, num, config)
			mockup_us = float(Time.get_ticks_usec() - t0) / n - baseline_us
			save_overhead_csv(path, "noop_mockup", hm.get_size(), run_idx, mockup_us)
		
		print("BASELINE: " + str(baseline_us))
		print("NO ARGS: " + str(noargs_us))
		print("INT: " + str(int_us))
		print("FLOAT: " + str(float_us))
		print("ARRAY: " + str(array_us))
		print("MOCKUP: " + str(mockup_us))

func save_overhead_csv(path: String, variant: String, array_size: int, run_idx: int, per_call_ns: float) -> void:
	var file_acc: FileAccess = FileAccess.open(path, FileAccess.READ_WRITE)
	file_acc.seek_end()
	file_acc.store_line("%s,%d,%d,%f" % [variant, array_size, run_idx, per_call_ns])
	file_acc.close()
