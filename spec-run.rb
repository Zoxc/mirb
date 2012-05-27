require_relative 'spec'

ARGV.unshift '-f', 's' 

puts ARGV.inspect
load "mspec/bin/mspec-run"