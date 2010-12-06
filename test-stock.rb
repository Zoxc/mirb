require 'benchmark'

module Kernel
	def benchmark
		Benchmark.measure { yield }
	end
	
	def backtrace
		caller.join("\n")
	end
end

require_relative 'test-bench'
gets