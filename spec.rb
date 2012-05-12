$:.unshift File.expand_path('speclib', File.dirname(__FILE__))

require 'enumerable'
require 'fileutils'

load File.expand_path("mspec/bin/mspec-run", File.dirname(__FILE__))