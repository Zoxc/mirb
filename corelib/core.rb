current_directory = File.dirname(__FILE__)

$:.unshift File.expand_path('../rubylib', current_directory)

RUBY_PATCHLEVEL = 0

$stderr = Object.new
$stdout = Object.new
$stdin = Object.new

module Process
	def self.pid
		1
	end
end

class File
	if RUBY_PLATFORM.match(/winapi/)
		FNM_SYSCASE = 8
		ALT_SEPARATOR =  '\\'
	else
		FNM_SYSCASE = 0
		ALT_SEPARATOR = nil
	end
end

module Signal
	def self.trap(name)
	end
end

Struct = Class

ENV = {}
ENV['HOME'] = File.expand_path('../home', current_directory)

require File.join(current_directory, 'enumerable')