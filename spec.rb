$:.unshift File.expand_path('speclib', File.dirname(__FILE__))

RUBY_VERSION = 0.1
RUBY_PATCHLEVEL = 0

class File
	if RUBY_PLATFORM.match(/winapi/)
		FNM_SYSCASE = 8
		ALT_SEPARATOR =  '\\'
	else
		FNM_SYSCASE = 0
		ALT_SEPARATOR = nil
	end
end

require 'enumerable'
require 'fileutils'

load File.expand_path("mspec/bin/mspec-run", File.dirname(__FILE__))