#!/usr/bin/ruby

class Node
		attr_accessor :dir, :symbol, :freq, :encodedPath, :left, :right, :parent

		def initialize(dir,symbol,freq,left,right,parent)
			@dir = 0
			@symbol = symbol
			@freq = freq
			@left = left
			@right = right
			@parent = parent
		end
end

class Huffman
end

node = Node.new(0,"a",0,nil,nil,nil)
puts node.symbol
