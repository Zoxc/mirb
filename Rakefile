require_relative '../reno-0.2/reno'

include Reno

output = 'mirb'

package = Package.new do
	# name and version
	name 'mirb'
	version '0.1.0'

	windows = Rake::Win32.windows?
	vendor = 'vendor' + (windows ? '' : '_linux')
	debug = false
	debug_info = false
	no_gc = false
	valgrind = false
	clang = false
	
	# setup toolchain
	
	set Toolchain::Optimization, debug ? :none : :balanced
	set Toolchain::Exceptions, :none
	set Toolchain::StaticLibraries, false
	
	set Toolchain::DebugInformation, debug
	
	set Toolchain::Libraries, vendor + '/gc/.libs/gc' unless valgrind || no_gc
	set Toolchain::Libraries, vendor + '/udis86/libudis86/.libs/udis86'
	
	set Toolchain::Libraries, 'pthread' unless windows || valgrind
	
	set Languages::C::Includes, [vendor, vendor + '/udis86']
	
	if clang
		use Toolchain::LLVM
		use Toolchain::GNU::Assembler
		use Toolchain::GNU::Linker
	else
		use Toolchain::GNU
	end
	
	# languages
	use Assembly::WithCPP
	c = use Languages::C
	c.std 'c99'
	c.define('WIN_SEH') if windows
	c.define('VALGRIND') if valgrind
	c.define('NO_GC') if no_gc
	c.define 'DEBUG' if debug_info
	cxx = use Languages::CXX
	cxx.std 'c++0x'
	
	# files
	files = collect('main.cpp', 'runtime/**/*.cpp', 'src/**/*.cpp')
	
	files += collect('runtime/**/*.S') unless debug_info
	
	files.merge(Executable).name(output, windows)
end

task :docs do
	require 'redcloth'
	files = Dir['docs/**/*.redcloth']
	files.each do |file|
		input = File.open(file, 'r') { |file| file.read }
		File.open(file.ext('.html'), 'w') { |file| file.write RedCloth.new(input).to_html }
	end
end

task :build do
	package.run
end

task :default => :build
