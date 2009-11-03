#!/usr/bin/env ruby

require "../common"

topDir = File.dirname(File.expand_path(__FILE__))

parseArgs()

name = "CLucene"
version = "0.9.16a"
pkg = "clucene-contrib-#{version}"
tarball = "#{pkg}.tar.bz2"
md5 = "ba1a8f764a2ca19c66ad907dddd88352"
pkgDir = File.join(topDir, pkg)

url = "http://downloads.sourceforge.net/project/clucene/clucene-contribs-unstable/#{version}/#{tarball}"

buildDir = File.join(topDir, "build")

# Make a stab at determining if we need to build.
if !buildNeeded?("#{$headerInstallDir}/magick/magick.h")
  puts "#{name} doesn't need to be built"
  exit 0
end

# Start with a clean slate
#
puts "removing previous build artifacts..."
FileUtils.rm_rf(pkgDir)
if $clean
    FileUtils.rm_f(tarball)
    exit 0
end

# Fetch distro if needed
#
fetch(tarball, url, md5)

# unpack distro and patch
#
unpack(tarball)
applyPatches(pkgDir)

err = nil
begin
  Dir.chdir(pkgDir) do
    puts "Building #{name}..."

    if $platform == "Windows"
      Dir.chdir("projects/visualc71") do
        suffix = bt == "Debug" ? "d" : ""
        system("devenv libpng.sln /build \"LIB #{bt}\"")
        FileUtils.install("Win32_LIB_#{bt}/libpng#{suffix}.lib",
                          "#{$libInstallDir}/#{bt}/libpng.lib",
                          :verbose => true)
      end
    else
      ENV['CFLAGS'] = ENV['CFLAGS'].to_s + $darwinCompatCompileFlags
      ENV['CXXFLAGS'] = ENV['CXXFLAGS'].to_s + $darwinCompatCompileFlags
      system("sh ./configure --enable-static --disable-shared --prefix=#{$installDir} --enable-ascii --with-clucene=../../Darwin/")
      system("make install")

      # for some reason, the clucene make install step drops clucene-config.h
      # into lib/ !?  let's copy it!
#      FileUtils.cp(File.join($libInstallDir, "CLucene", "clucene-config.h"),
#                   File.join($headerInstallDir, "CLucene"))
    end
  end
rescue RuntimeError => err
end

raise "#{name} build error: " + err if err != nil

if !$keepSource
    puts "removing build artifacts..."
    FileUtils.rm_rf(pkgDir) 
end
