// BindReflection.h
//------------------------------------------------------------------------------
#ifndef CORE_BINDREFLECTION_H
#define CORE_BINDREFLECTION_H

// ForwardDeclaration
//------------------------------------------------------------------------------

// Macro to force a reference to a class
//------------------------------------------------------------------------------
#define BIND_REFLECTION( className ) \
	extern void className##_ReflectionInfo_Bind(); \
	className##_ReflectionInfo_Bind();

// BindReflection_Core
//------------------------------------------------------------------------------
void BindReflection_Core();

//------------------------------------------------------------------------------
#endif // CORE_BINDREFLECTION_H
