#pragma once

// Limiter only needs the live sender object's address in order to hook its
// send-to-server vtable entry. No native methods are called through this type.
class LoopbackPacketSender {};
