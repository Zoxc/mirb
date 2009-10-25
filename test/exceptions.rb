test "Exception handling", 1 do
	begin
		1
	rescue
		2
	end
end

test "Rescue block", 2 do
	begin
		raise
	rescue
		2
	end
end

test "Ensure block", 2 do
	x = 1
	begin
		begin
			raise
		ensure
			x = 2
		end
	rescue
	end
	x
end