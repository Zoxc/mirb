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

test "Next skips code", 1 do
	a = 1
	tap do
		next 4
		a = 2
	end
	a
end

test "Redo restarts block", 2 do
	a = 1
	pass = 0
	proc do
		pass += 1
		
		if pass == 2
			a = 2
		end
		
		redo if pass != 2
	end.call
	a
end

test "Redo re-evalutes params", 1 do
	pass = 0
	block = proc do |x|
		pass += 1

		if pass == 1
			x = 3
		end
		
		redo if pass != 2
		
		x
	end.call(1)
end