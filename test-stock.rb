require 'benchmark'

module Kernel
	def benchmark
		Benchmark.measure { yield }
	end
end

require_relative 'test'
gets