#!/usr/bin/ruby

file = File.open("Amazon_EC2.txt", "r")
contents = file.read
tok = contents.split('')

#Count total number of characters
total = 0
tok.each do |letter|
	if letter =~ /[A-Za-z]/
		total += 1
	end
end

#Iterate through each character and count if it exists in the file
#Not case sensitive
('A'..'Z').each do |letter|
	count = 0
	tok.each do |l|
		if l.upcase == letter
			count += 1
		end
	end
	print "Letter #{letter} " 
	#Print frequency
	ratio = ( count.to_f / total.to_f ) * 100
	puts ratio
end
