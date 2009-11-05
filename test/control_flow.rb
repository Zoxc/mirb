test "Break returns value", 2 do
	tap do
		break 2
		1
	end
end

test "Break returns nil", nil do
	tap do
		break
		1
	end
end