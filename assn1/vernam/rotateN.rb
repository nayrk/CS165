#!/usr/bin/ruby

if ARGV.size == 0 || ARGV.size > 1
	puts "Enter only 1 argument from 0-26 for alpha shift"
	exit
end

count = 1
$hashes = {}
('a'..'z').each do |i|
	$hashes["#{i}"] = count
	count += 1
end

letter = "a"
(1..26).each do |i|
	$hashes[i] = letter
	letter = letter.next
end

#Not case sensitive
def rot13(string)
	encrypted = ''
	string = string.split('')
	string.each do |l|
		if l != ' ' && l =~ /[A-Za-z]/
			l = l.downcase
			position = $hashes["#{l}"].to_i + ARGV[0].to_i
			if position > 26
				position = position - 26
			end
			print $hashes[position]
			encrypted = encrypted << $hashes[position]
		else
			encrypted = encrypted << l
			print l
		end
	end
	return encrypted
end

print "Enter a string to encode: "
string = STDIN.gets
puts
puts "Encrypted"
#string="Max ybex bl t lmtgwtkw IhlmLvkbim ybex xgvkrimxw pbma t oxkgtf vbiaxk nlbgz t ebgxtk vhgzknxgmbte zxgxktmhk. xtva uehvd bl xqtvmer lbqmxxg ubml ehgz"
encrypted = rot13(string)

#the file is a standard postscript file encrypted with a vernam cipher using a linear congruential
#generator. each block is exactly sixteen bits long


#string = "How can you tell an extrovert from an introvert at NSA? Va gur ryringbef, gur rkgebireg ybbxf ng gur BGURE thl'f fubrf."

