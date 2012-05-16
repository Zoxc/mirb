$:.unshift File.expand_path('speclib', File.dirname(__FILE__))

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
	load File.expand_path("mspec/bin/mspec-run", File.dirname(__FILE__))
}