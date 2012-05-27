
module RbConfig
  CONFIG = {}
  CONFIG["RUBY_INSTALL_NAME"] = 'mirb'
  CONFIG["EXEEXT"] = RUBY_PLATFORM.match(/winapi/) ? '.exe' : ''
  CONFIG["bindir"] = File.expand_path('../../', __FILE__)
end
