require 'rake/package'

begin
	require 'rake/env'
rescue LoadError
end

MIRB = Builder::Package.new('mirb', '.', [
	'main.c',
	'compiler/**/*.c',
	'runtime/**/*.c'
])

def execute(target, settings)
	if Rake::Win32.windows?
		settings = Builder::Settings.new(settings.name, settings, {:CFLAGS => ['-DWINDOWS', '-I','vendor/BeaEngine'], :LDFLAGS => ['-L', 'vendor/BeaEngine'], :LIBS => ['BeaEngine']})
	end

	unless MIRB.send target, settings
		puts "Nothing to do!"
	end
end

desc "Delete all the generated files"
task :clean do
	execute(:clean, Builder::Preset::RELEASE)
	execute(:clean, Builder::Preset::DEBUG)
	puts "It's nice and clean here."
end

desc "Build an optimized version of #{MIRB.name}"
task :release do
	execute(:build, Builder::Preset::RELEASE)
end

desc "Build a debug version of #{MIRB.name}"
task :debug do
	execute(:build, Builder::Preset::DEBUG)
end

desc "Convert all spaces to tabs"
task :tabs do
	MIRB.sources.map { |source| FileList[source] }.flatten.each do |file|
		output = Builder.execute('unexpand', '--tabs=4', file)
		File.open(file, 'w') { |file| file.write(output) }
	end
end

task :default => :release