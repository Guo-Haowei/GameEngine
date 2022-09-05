-- This is a short comment

function main()
    local a = 1 == 0xABC
    local b = (1 << 1) + 0xFE
    local c = 12.2 * 2.8e+1 - 0x12p+2
    local d = .2 + .3
    print('hello')
    print("a: " .. tostring(a))
    print("b: " .. tostring(b))
    print("c: " .. tostring(c))
    print("d: " .. tostring(d))
end

main()