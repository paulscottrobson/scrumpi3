

code = {}
mnemonics = {}
execute = {}

function process(str,opcode)
	local sub = ("%d"):format(opcode % 4)
	str = str:gsub("%%P",sub)
	if opcode % 4 == 0 then sub = "P" end
	str = str:gsub("%%O",sub)
	return str
end

src = io.open("scmp.def","r")
for l in src:lines() do		
	if l:match("//") then l = l:match("^(.*)//") end
	l = l:match("^%s*(.*)$"):gsub("%s"," ")
	while l:match(" $") do l = l:sub(1,-2) end
	if #l > 0 then
		if l:match("^%:") then
			code[#code+1] = l:sub(2)
		else
			range,cycles,mnemonic,source = l:match('^([%x%-]+)%s+(%d+)%s+"([%|%w%@%s%%%(%)%*]+)"%s+(.*)$')
			assert(source ~= nil)
			if #range == 2 then range = range.."-"..range end
			first = tonumber(range:sub(1,2),16)
			last = tonumber(range:sub(4,5),16)
			for op = first,last do
				assert(mnemonics[op] == nil,op)
				mnemonics[op] = process(mnemonic,op):lower()
				execute[op] = "addCycles(" .. cycles .. ');' .. process(source,op)
			end
		end
	end
end
src:close()

for i = 0,255 do
	if mnemonics[i] == nil then mnemonics[i] = "" end
end

hCode = io.open("scmp_autocode.h","w")
for _,l in ipairs(code) do hCode:write(l.."\n") end
hCode:close()

hMnemonics = io.open("scmp_mnemonics.h","w")
mnelist = '"'..table.concat(mnemonics,'","',0,255)..'"'
hMnemonics:write("static char *_mnemonics[256] = { "..mnelist.."};")
hMnemonics:close()

hCase = io.open("scmp_opcodes.h","w")
for i = 0,255 do
	if execute[i] ~= nil then
		hCase:write(("case 0x%02x: /* %s */\n"):format(i,mnemonics[i]))
		hCase:write("    " .. execute[i]..";break;\n")
	end
end
hCase:close()