require_relative 'test'
puts "Benchmark #{benchmark { 10000.times { silent_run! } } }"
