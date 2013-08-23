require_relative "iphone/Scripts/RakeHelper.rb"
require "xcoder"
require "spinning_cursor"

## Build Tasks ##

task :default => 'build:lib'
task :build => 'build:lib'

namespace :build do

  desc "Build and Link libzbar.a"
  task :lib do
    
    puts
    puts "Building libzbar for iphoneos".cyan
    
    config = Xcode.project(:zbar).target(:libzbar).config(:Release)
    builder = config.builder
    builder.clean
    builder.build
    
    puts 
    puts "Building libzbar for iphonesimulator".cyan
    
    builder = Xcode.project(:zbar).scheme("libzbar").builder
    builder.build :sdk => :iphonesimulator
    
    puts
    sdkDir = "ZBarSDK"
    source_path = "iphone"
    Runner.instance.execute "Cleaning old #{sdkDir} folder", "rm -r ZBarSDK"
    Runner.instance.execute "Creating #{sdkDir} folder", "mkdir #{sdkDir}"
    Runner.instance.execute "Copying Resources into #{sdkDir}", "cp -R #{source_path}/res #{sdkDir}/Resources"
    Runner.instance.execute "Copying Headers into #{sdkDir}", "cp -R #{source_path}/include/ZBarSDK #{sdkDir}/Headers"
    Runner.instance.execute "Linking libzbar into #{sdkDir}", "lipo -create #{source_path}/build/Release-iphoneos/libzbar.a #{source_path}/build/Release-iphonesimulator/libzbar.a -o #{sdkDir}/libzbar.a"
    
  end

end