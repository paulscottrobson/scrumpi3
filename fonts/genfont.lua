function convertToBinary(s) 
	local bit = 128
	local n = 0
	while s ~= "" do
		if s:sub(1,1) >= "A" then n = n + bit end
		bit = bit / 2
		s = s:sub(2)
	end
	return n
end

function fileChars(binaryDataSmall,binaryDataLarge) 
	local h = io.open("font.txt","r")
	local charNumber = 0
	local charPositionSmall = 0
	local charPositionLarge = 0
	assert(h ~= nil)
	for line in h:lines() do
		while line:sub(1,1) == " " do line = line:sub(2) end
		if line ~= "" and line:sub(1,1) ~= '/' then
			local m1,m2 = line:match("^([%w%.]+)%s*([%w%.]*)")
			if m1:match("^(%d+)$") then
				charNumber = tonumber(m1)
				charPositionLarge = 0
				charPositionSmall = 0
			else
				--print (charNumber,'('..(m1 or "")..'|'..(m2 or "")..')')
				binaryDataLarge[charNumber*9+charPositionLarge] = convertToBinary(m1)
				charPositionLarge = charPositionLarge + 1
				assert(charPositionLarge <= 9)
				if m2 ~= "" then
					binaryDataSmall[charNumber*5+charPositionSmall] = convertToBinary(m2)
					charPositionSmall = charPositionSmall + 1
					assert(charPositionSmall <= 5)
				end
			end
		end
	end
end

function dumpROM(binaryData,includeFile,constName) 
	print("Converting to "..includeFile)
	local h = io.open(includeFile,"w")	
	assert(h ~= nil,"Can't write "..includeFile)
	h:write("static BYTE8 "..constName.."[] = {\n")
	for n = 0,binaryData.size-1 do
		assert(binaryData[n] ~= nil)
		h:write(("0x%02x"):format(binaryData[n]))
		if n ~= binaryData.size-1 then h:write(",") end
		if n % 16 == 15 then h:write("\n") end
	end
	h:write("};\n")
	h:close()
end

local fontROMSmall = { size = 5 * 64 }
local fontROMLarge = { size = 9 * 64 }
fileChars(fontROMSmall,fontROMLarge)
dumpROM(fontROMLarge,"font_large.h","_largeFont")
dumpROM(fontROMSmall,"font_small.h","_smallFont")
print("Complete.")