local m = Map("remotewol", translate("远程唤醒配置"))

local s = m:section(TypedSection, "settings", translate("设置"))
s.addremove = false
s.anonymous = true

local srvport = s:option(Value, "port", translate("端口"))
srvport.datatype = "port"
srvport.rmempty = false

local status = s:option(DummyValue, "_status", translate("服务状态"))
status.dynamic = false

local list = m:section(TypedSection, "devices", translate("设备列表"))
list.template = "cbi/tblsection"
list.addremove = true
list.anonymous = true

local ip = list:option(Value, "ip", translate("IP 地址"))
ip.datatype = "ipaddr"

local mac = list:option(Value, "mac", translate("MAC 地址"))
mac.datatype = "macaddr"

local command = list:option(Value, "command", translate("命令"))
command.datatype = "string"

function status.cfgvalue(self, section)
    local pid = luci.sys.exec("pidof remotewol")
    if pid and #pid > 0 then
        return translate("运行中")
    else
        return translate("未运行")
    end
end

--点击保存并应用后，重启服务
m.on_after_commit = function()
    luci.sys.call("/etc/init.d/remotewol restart")
end

return m