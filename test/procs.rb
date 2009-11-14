test "Upval", 2 do
	a = 1; b = proc { a = 2 }; b.call; a
end

test "Nested upval", 2 do
	a = 1; b = proc { proc { a = 2 } }; b.call.call; a
end

test "Closed upval", 6 do
	def closed_upval_gen a; proc { a } end; a = closed_upval_gen 6; a.call
end

test "Block callback", 3 do
	def block_callback_do; yield end; block_callback_do { 1 + 2 }
end

test "Block callback with parameters", 2 do
	proc { |x, y| x + y }.call(1, 1)
end
