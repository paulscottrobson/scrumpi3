--
--		ROM Process. Produces 8k dump irrespective of size.
--

function loadROM(binaryData,binaryFile,offset) 
	if binaryData.size == nil then binaryData.size = 0 end
	local bin = io.open(binaryFile,"rb")
	assert(bin ~= nil,"Can't read "..binaryFile)
	repeat
		local s = bin:read(1)
		if s ~= nil then
			local sb = string.byte(s)
			binaryData[offset] = sb
			offset = offset + 1
			if offset > binaryData.size then binaryData.size = offset end
		end
 	until s == nil
	bin:close()

end

function dumpROM(binaryData,includeFile,constName,patch) 
	patch =  patch or {}

	for addr,patchList in pairs(patch) do
		for _,patch in ipairs(patchList) do
			image[addr] = patch
			addr = addr + 1
		end
	end

	local h = io.open(includeFile,"w")
	assert(h ~= nil,"Can't write "..includeFile)
	h:write("static BYTE8CONST "..constName.."[] = {\n")
	for n = 0,binaryData.size-1 do
		h:write(("0x%02x"):format(binaryData[n]))
		if n ~= binaryData.size-1 then h:write(",") end
		if n % 16 == 15 then h:write("\n") end
	end
	h:write("};\n")
	h:close()
end

function fillRom(binaryData,size) 
	for i = 0,size-1 do binaryData[i] = 0x55 if i % 2 == 0 then binaryData[i] = 0xAA end end
	binaryData.size = size
end

local biosROM = {}
fillRom(biosROM,2048)
loadROM(biosROM,"bios.bin",0x0000);
print(biosROM.size)
dumpROM(biosROM,"scrumpi3_bios.h","_biosROM")
