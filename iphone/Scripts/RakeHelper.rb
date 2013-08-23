# encoding: utf-8
require "open4"
require "spinning_cursor"

class String
  def self.colorize(text, color_code)
    "\e[#{color_code}m#{text}\e[0m"
  end

  def red
    self.class.colorize(self, 31);
  end

  def green
    self.class.colorize(self, 32);
  end

  def yellow
    self.class.colorize(self, 33);
  end

  def cyan
    self.class.colorize(self, 36);
  end

  def grey
    self.class.colorize(self, 37);
  end
end

class Runner
  attr_accessor :verbose

  def self.instance
    @instance ||= Runner.new
  end

  def initialize
    @verbose = ENV['VERBOSE'] || false
  end

  def verbose?
    self.verbose
  end

  def execute(title, command)
    if verbose?
      bannerTitle = "- #{title} :: #{command}"
    else
      bannerTitle = "- #{title}"
    end

    SpinningCursor.start do
      banner bannerTitle.cyan
      message "✓ #{title}".green
    end

    status = Open4::popen4(command) do |pid, stdin, stdout, stderr|
      stdout.each_line do |line|
        if verbose?
          puts line.yellow
        end
      end
      stderr.each_line do |line|
        SpinningCursor.set_message "✘ #{line}".red
      end
    end

    SpinningCursor.stop
    return status.exitstatus
  end
end