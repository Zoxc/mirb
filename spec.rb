current_directory = File.dirname(__FILE__)

$:.unshift File.expand_path('speclib', current_directory)

RUBY_PATCHLEVEL = 0

class SystemExit < Exception
end

module Kernel
	def exit(errnum)
		raise SystemExit
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

require 'enumerable'
require 'fileutils'
require 'pp'

ARGV << ':core' << ':language'

puts benchmark {
	Dir.chdir(File.expand_path('rubyspec', current_directory)) do
		load File.expand_path("mspec/bin/mspec-run", current_directory)
	end
}