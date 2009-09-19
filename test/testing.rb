module Testing
	Tests = []
	
	class Test
		def initialize name, expect, proc
			@name = name
			@expect = expect
			@proc = proc
		end
		
		def pass?
			result = @proc.call
			if @expect != result
				print "#{@name} failed. Excepted #{@expect.inspect}, but got #{result.inspect}.\n"
			else
				print "#{@name} passed.\n"
				true
			end
		end
	end
	
	def test name, expect, &proc
		Tests.push(Test.new(name, expect, proc))
	end
	
	def run!
		failed = 0
		
		Tests.each do |test|
			failed += 1 if !test.pass?
		end
		
		if failed == 0
			print "All #{Tests.length} test#{"s" if Tests.length != 1} passed successfully.\n"
		else
			print "#{failed} of #{Tests.length} failed.\n"
		end
	end
end