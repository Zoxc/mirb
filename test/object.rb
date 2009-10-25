test "Object#tap returns self", 2 do
	2.tap {|x| 1}
end

test "Object#tap calls block", 2 do
	begin
		1.tap {|x| raise}
	rescue
		2
	end
end
