#!/usr/bin/env ruby

abort "Usage: #{$0} <input_png_file> <output_c_file>" if ARGV.length != 2
abort "ERROR: File #{ARGV[1]} allready exists!" if File.exist?(ARGV[1])

cmd = "ffmpeg -v error -i \"#{ARGV[0]}\" -f rawvideo -pix_fmt bgra -y \"#{ARGV[0]}.raw\""
abort unless system(cmd)
c = File.binread("#{ARGV[0]}.raw").unpack('L<*')

s = `ffprobe -v error -select_streams v -show_entries stream=width,height -of csv=p=0:s=x "#{ARGV[0]}"`
match = s.match(/^(\d+)x(\d+)$/)
abort 'ERROR: Cannot get image width and height' if match.nil?
width  = match[1].to_i
height = match[2].to_i

word_per_line = 8
last_word = c.length - 1
name = File.basename(ARGV[1], '.*').gsub(/[\s-]+/, '_')

File.open(ARGV[1], 'w') do |f|
  f.puts '#include "sprite.h"'
  f.puts
  f.puts "static const uint32_t #{name}_pixels[#{c.length}] = {"
  c.each_with_index do |v, i|
    f.write "\t" if (i % word_per_line) == 0
    f.write "0x#{v.to_s(16).rjust(8, '0')}"
    f.write ',' unless i == last_word
    if (i % word_per_line) == (word_per_line - 1) or i == last_word then
      f.write "\n"
    else
      f.write ' '
    end
  end
  f.puts '};'
  f.puts
  f.puts "const sprite_t #{name} = {"
  f.puts "\t.width=#{width},"
  f.puts "\t.height=#{height},"
  f.puts "\t.pixel_data=#{name}_pixels,"
  f.puts "};\n\n"
end

system("rm \"#{ARGV[0]}.raw\"")

