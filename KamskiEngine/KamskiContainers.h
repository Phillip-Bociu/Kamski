#pragma once
#include "KamskiEngine.h"
#include <cstring>
#include <utility>
#include <queue>

#ifndef KAMSKI_MAX_ENTITY_COUNT
#define KAMSKI_MAX_ENTITY_COUNT 10000
#endif

//TODO (phillip): replace templates with code generator

class IDStack
{
public:

    IDStack()
    {
        memset(this, sizeof(IDStack), 0);
    }

    void push(Entity eId)
    {
        assert(top != KAMSKI_MAX_ENTITY_COUNT);
        ids[top] = eId;
        top++;
    }

    void pop()
    {
        assert(top != 0);
        top--;
    }

    bool empty() const
    {
        return top == 0;
    }

    Entity front() const
    {
        assert(!empty());
        return ids[top-1];
    }

private:
    u32 top;
    Entity ids[KAMSKI_MAX_ENTITY_COUNT];
};

template<typename Component>
class ComponentView
{
    public:
    ComponentView(Component* begin, Component* end)
    {
        b = begin;
        e = end;
    }

    Component* begin()
    {
        return b;
    }

    Component* end()
    {
        return e;
    }

    const Component* begin() const
    {
        return b;
    }

    const Component* end() const
    {
        return e;
    }

private:
    Component* b;
    Component* e;
};

class EntityView
{
public:
    EntityView(Entity* const begin, Entity* const end) :
    b(begin), e(end)
    {
    }

    Entity* begin()
    {
        return b;
    }

    Entity* end()
    {
        return e;
    }

    [[nodiscard]]
    const Entity* begin() const
    {
        return b;
    }

    [[nodiscard]]
        const Entity* end() const
    {
        return e;
    }

private:
    Entity* b;
    Entity* e;
};


template<typename Component>
class ComponentVector
{
    public:

    void clear()
    {
        denseSize = 0;
    }

    template<typename ... Args>
    Component& addComponent(const Entity eId, Args&& ... args)
    {
        assert(eId < sparseSize);
        assert(denseSize != denseCapacity);

        sparse[eId] = denseSize;
        dense[denseSize] = eId;
        compArray[denseSize] = Component{ std::forward<Args>(args)... };
        return compArray[denseSize++];
    }

    [[nodiscard]]
    bool hasComponent(const Entity eId) const
    {
        const auto teId = eId;
        const auto ss = sparseSize;
        const auto ds = denseSize;
        const auto sparseIndex = sparse[eId];
        const auto denseEntity = dense[sparse[eId]];

        return !(eId >= sparseSize || denseSize <= sparse[eId] || dense[sparse[eId]] != eId);
    }

    void removeComponent(Entity eId)
    {
        if (!hasComponent(eId))
        {
            return;
        }

        const u32 toRemoveIndex = sparse[eId];
        const u32 lastIndex = denseSize - 1;

        Entity lastEntity = dense[lastIndex];
        dense[toRemoveIndex] = lastEntity;
        sparse[lastEntity] = toRemoveIndex;
        compArray[toRemoveIndex] = compArray[lastIndex];
        denseSize--;
    }

    // Crashes if entity[eId] doesn't have this Component
    Component& getComponent(const Entity eId)
    {
        assert(hasComponent(eId));
        return compArray[sparse[eId]];
    }

    // Crashes if entity[eId] doesn't have this Component
    const Component& getComponent(const Entity eId) const
    {
        assert(hasComponent(eId));
        return compArray[sparse[eId]];
    }

    // Ranged for functions

    ComponentView<Component> iterateComponents()
    {
        return ComponentView<Component>((Component*)compArray, (Component*)compArray + denseSize);
    }

    const ComponentView<Component> iterateComponents() const
    {
        return ComponentView<Component>((Component*)compArray, (Component*)compArray + denseSize);
    }

    EntityView iterateEntities()
    {
        return EntityView{ (Entity*)dense, (Entity*)dense + denseSize };
    }

    const EntityView iterateEntities() const
    {
        return EntityView((Entity*)dense, (Entity*)dense + denseSize);
    }

    u64 size() const
    {
        return denseSize;
    }

    private:

    static constexpr u32 sparseSize = KAMSKI_MAX_ENTITY_COUNT;
    static constexpr u32 denseCapacity = KAMSKI_MAX_ENTITY_COUNT;

    u32 sparse[sparseSize];
    Entity dense[denseCapacity];
    Component compArray[denseCapacity];

    u32 denseSize;
};


template<typename ... T>
struct ComponentList;

template<>
struct ComponentList<>
{
    static constexpr u64 size = 0;
    using CurrentType = void;

    void removeEntity(Entity eId)
    {
    }

    template<typename Component>
    static constexpr u64 componentId()
    {
        return 0;
    }

    template<typename Component>
    ComponentVector<Component>& getComponentVector()
    {
        ComponentVector<Component> errVec = {};
        assert(false && "Component not in list");
        return errVec;
    }

    template<typename Component>
    const ComponentVector<Component>& getComponentVector() const
    {
        ComponentVector<Component> errVec = {};
        assert(false && "Component not in list");
        return errVec;
    }

};

template<typename FirstType, typename ... Types>
struct ComponentList<FirstType, Types ...> : public ComponentList<Types ...>
{
    static constexpr u64 size = 1 + sizeof ... (Types);
    using CurrentType = FirstType;

    template<typename Component>
    static constexpr u64 componentId()
    {
        if constexpr(std::is_same_v<Component, CurrentType>)
        {
            return 0;
        }
        else
        {
            using next = ComponentList<Types ...>;
            return 1 + next::template componentId<Component>();
        }
    }

    void removeEntity(Entity eId)
    {
        cvector.removeComponent(eId);
        using next = ComponentList<Types ...>;
        next::removeEntity(eId);
    }


    template<typename Component>
    ComponentVector<Component>& getComponentVector()
    {
        if constexpr(std::is_same_v<Component, CurrentType>)
        {
            return cvector;
        } else
        {
            static_assert(sizeof...(Types) != 0);
            using next = ComponentList<Types ...>;
            return next::template getComponentVector<Component>();
        }
    }

    template<typename Component>
    const ComponentVector<Component>& getComponentVector() const
    {
        if constexpr(std::is_same_v<Component, CurrentType>)
        {
            return cvector;
        } else
        {
            static_assert(sizeof...(Types) != 0);
            using next = ComponentList<Types ...>;
            return next::template getComponentVector<Component>();
        }
    }

    ComponentVector<FirstType> cvector;
};

template<typename _ComponentList>
struct sig_t
{
    static constexpr u64 SIG_SIZE = sizeof(u64) * 8;
    static constexpr u64 BIT_COUNT = _ComponentList::size;
    static constexpr u64 ACTUAL_BIT_COUNT = (BIT_COUNT + (SIG_SIZE - (BIT_COUNT % SIG_SIZE) % SIG_SIZE)) / SIG_SIZE;

    u64 bytes[ACTUAL_BIT_COUNT];
    constexpr bool operator==(const sig_t& other) const
    {
        for (u64 i = 0; i < ACTUAL_BIT_COUNT; i++)
        {
            if (other.bytes[i] != bytes[i])
                return false;
        }
        return true;
    }

    constexpr bool operator!=(const sig_t& other) const
    {
        for (u64 i = 0; i < ACTUAL_BIT_COUNT; i++)
        {
            if (other.bytes[i] != bytes[i])
                return true;
        }
        return false;
    }

    constexpr sig_t operator|(const sig_t& other) const
    {
        sig_t retval = {};
        for (u64 i = 0; i < ACTUAL_BIT_COUNT; i++)
        {
            retval.bytes[i] = bytes[i] | other.bytes[i];
        }
        return retval;
    }

    constexpr sig_t operator&(const sig_t& other) const
    {
        sig_t retval = {};
        for (u64 i = 0; i < ACTUAL_BIT_COUNT; i++)
        {
            retval.bytes[i] = bytes[i] & other.bytes[i];
        }
        return retval;
    }

};

template<typename _ComponentList>
struct Signature
{
    sig_t<_ComponentList> signature;
    Entity eId;

    static constexpr u64 componentIdToBit(u32 id)
    {
        return (u64)1 << (u64)id % sig_t<_ComponentList>::SIG_SIZE;
    }

    static constexpr u32 componentIdToByte(u32 id)
    {
        return id / sig_t<_ComponentList>::SIG_SIZE;
    }

    template<typename Component>
    constexpr void addComponent()
    {
        u64 cId = _ComponentList::template componentId<Component>();
        signature.bytes[componentIdToByte(cId)] |= componentIdToBit(cId);
    }


    template<typename Component>
    constexpr void removeComponent()
    {
        u64 cId = _ComponentList::template componentId<Component>();
        signature.bytes[componentIdToByte(cId)] &= ~componentIdToBit(cId);
    }

    template<typename Component>
    bool hasComponent() const
    {
        u64 cId = _ComponentList::template componentId<Component>();
        return (signature.bytes[componentIdToByte(cId)] & componentIdToBit(cId)) != 0;
    }

};


template<typename _ComponentList, typename FirstComponent, typename ... NextComponents>
struct TSignature
{

    static constexpr u64 SIG_SIZE = sizeof(u64) * 8;
    static constexpr u64 BIT_COUNT = _ComponentList::size;
    static constexpr u64 ACTUAL_BIT_COUNT = (BIT_COUNT + (SIG_SIZE - (BIT_COUNT % SIG_SIZE) % SIG_SIZE)) / SIG_SIZE;

    static constexpr u64 componentIdToBit(u32 id)
    {
        return (u64)1 << (u64)id % SIG_SIZE;
    }

    static constexpr u64 componentIdToByte(u32 id)
    {
        return id / SIG_SIZE;
    }

    template<typename ... Next>
    struct get_sig_t;

    template<>
    struct get_sig_t<>
    {
        static constexpr sig_t<_ComponentList> _getSignature()
        {
            return {};
        }
    };

    template<typename First, typename ... Next>
    struct get_sig_t<First, Next ...>
    {
        static constexpr sig_t<_ComponentList> _getSignature()
        {
            using next = get_sig_t<Next ...>;
            sig_t<_ComponentList> retval = {};
            constexpr u64 cId = _ComponentList::template componentId<First>();
            retval.bytes[componentIdToByte(cId)] |= componentIdToBit(cId);
            return retval | next::_getSignature();
        }
    };

    static constexpr sig_t<_ComponentList> getSignature()
    {
        return get_sig_t<FirstComponent, NextComponents ...>::_getSignature();
    }
};

template<typename Sig, typename _ComponentList>
class SignatureIterator
{
    public:
    using Signature = Signature<_ComponentList>;

    SignatureIterator(Signature* ptr, Signature* end):
        ptr(ptr),
        end(end)
    {
    }

    Entity operator*() const
    {
        return ptr->eId;
    }

    SignatureIterator& operator++()
    {
        ptr++;
        constexpr sig_t<_ComponentList> sig = signature;
        while (ptr != end && (ptr->signature & signature) != signature)
        {
            ptr++;
        }
        return *this;
    }

    SignatureIterator operator++(int)
    {
        SignatureIterator temp = *this;
        ptr++;
        constexpr sig_t<_ComponentList> sig = signature;
        while (ptr != end && (ptr->signature & signature) != signature)
        {
            ptr++;
        }
        return temp;
    }

    bool operator==(const SignatureIterator& pther) const
    {
        return ptr == pther.ptr;
    }

    bool operator!=(const SignatureIterator& pther) const
    {
        return ptr != pther.ptr;
    }

    private:
    Signature* ptr;
    const Signature* end;
    static constexpr auto signature = Sig::getSignature();
};


template<typename Sig, typename _ComponentList>
class ConstSignatureIterator
{
    public:
    using Signature = Signature<_ComponentList>;

    ConstSignatureIterator(const Signature* ptr, const Signature* end):
        ptr(ptr),
        end(end)
    {
    }

    Entity operator*() const
    {
        return ptr->eId;
    }

    ConstSignatureIterator& operator++()
    {
        ptr++;
        while (ptr != end && (ptr->signature & signature) != signature)
        {
            ptr++;
        }
        return *this;
    }

    ConstSignatureIterator<Sig, _ComponentList> operator++(int)
    {
        auto temp = *this;
        ptr++;
        while (ptr != end && (ptr->signature & signature) != signature)
        {
            ptr++;
        }
        return temp;
    }

    bool operator==(const ConstSignatureIterator<Sig, _ComponentList>& other) const
    {
        return ptr == other.ptr;
    }

    bool operator!=(const ConstSignatureIterator<Sig, _ComponentList>& other) const
    {
        return ptr != other.ptr;
    }

private:
    const Signature* ptr;
    const Signature* end;
    static constexpr auto signature = Sig::getSignature();
};


template<typename Sig, typename _ComponentList>
class SignatureView
{
public:
    using Signature = Signature<_ComponentList>;

    SignatureView(Signature* _begin, Signature* _end):
    _end(_end)
    {
        constexpr sig_t<_ComponentList> sigResult = Sig::getSignature();

        while (_begin != _end)
        {
            sig_t<_ComponentList> res = (_begin->signature & sigResult);
            if (res != sigResult)
                _begin++;
            else
                break;
        }
        this->_begin = _begin;
    }

    SignatureIterator<Sig, _ComponentList> begin()
    {
        SignatureIterator<Sig, _ComponentList> ret(_begin,_end);
        return ret;
    }

    SignatureIterator<Sig, _ComponentList> end()
    {
        SignatureIterator<Sig, _ComponentList> ret(_end,_end);
        return ret;
    }

    ConstSignatureIterator<Sig, _ComponentList> begin() const
    {
        ConstSignatureIterator<Sig, _ComponentList> ret(_begin,_end);
        return ret;
    }

    ConstSignatureIterator<Sig, _ComponentList> end() const
    {
        ConstSignatureIterator<Sig, _ComponentList> ret(_end,_end);
        return ret;
    }

private:
    Signature* _begin;
    Signature* _end;
};

template<typename _ComponentList>
class EntityRegistry
{
public:
    using Signature = Signature<_ComponentList>;

    EntityRegistry():
    nextEntity(0)
    {

    }

    template<typename Component>
    ComponentVector<Component>& getComponentVector()
    {
        return components.template getComponentVector<Component>();
    }

    template<typename Component>
    const ComponentVector<Component>& getComponentVector() const
    {
        return components.template getComponentVector<Component>();
    }

    template<typename Component, typename ...  Args>
    Component& addComponent(Entity eId, Args&& ... args)
    {
        // Asserts maybe useless??
        assert(eId < nextEntity);
        assert(entityIndices[eId] < signatureCount);
        ComponentVector<Component>& cVec = getComponentVector<Component>();
        entitySignatures[entityIndices[eId]].template addComponent<Component>();
        return cVec.addComponent(eId, std::forward<Args>(args)...);
    }

    template<typename Component>
    Component& getComponent(Entity eId)
    {
        ComponentVector<Component>& cVec = getComponentVector<Component>();
        return cVec.getComponent(eId);
    }

    template<typename Component>
    const Component& getComponent(Entity eId) const
    {
        const ComponentVector<Component>& cVec = getComponentVector<Component>();
        return cVec.getComponent(eId);
    }

    template<typename Component>
    bool hasComponent(Entity eId) const
    {
        assert(eId != KAMSKI_MAX_ENTITY_COUNT);
        return entityIndices[eId] < signatureCount && entitySignatures[entityIndices[eId]].template hasComponent<Component>();
    }

    template<typename Component>
    void removeComponent(Entity eId)
    {
        assert(eId < KAMSKI_MAX_ENTITY_COUNT);
        ComponentVector<Component>& cVec = getComponentVector<Component>();
        const u32 componentId = getComponentId<Component>();
        entitySignatures[entityIndices[eId]].signature.template removeComponent<Component>();
        cVec.removeComponent(eId);
    }

    template<typename Component>
    void clear()
    {
        ComponentVector<Component>& cVec = getComponentVector<Component>();
        cVec.clear();
    }

    void removeMarkedEntities()
    {
        for (u32 i = signatureCount - markedBegin; i != signatureCount; i++)
        {
            removeEntity(entitySignatures[i].eId);
        }
        signatureCount -= markedBegin;
        markedBegin = 0;
    }

    Entity createEntity()
    {
        Entity retval = nextEntity;
        if (!previousIds.empty())
        {
            retval = previousIds.front();
            previousIds.pop();
        }
        else
        {
            assert(nextEntity != KAMSKI_MAX_ENTITY_COUNT);
            nextEntity++;
        }
        assert(signatureCount != signatureCapacity);
        entityIndices[retval] = signatureCount;
        entitySignatures[signatureCount] = {};
        entitySignatures[signatureCount].eId = retval;
        signatureCount++;
        return retval;
    }

    void markEntityForDeletion(Entity eId)
    {
        if (markedBegin == signatureCount || entityIndices[eId] > signatureCount - 1 - markedBegin)
        {
            return;
        }
        const u32 lastIndex = signatureCount - 1 - markedBegin;

        if (lastIndex != entityIndices[eId])
        {
            Signature aux = entitySignatures[lastIndex];
            entitySignatures[lastIndex] = entitySignatures[entityIndices[eId]];
            entitySignatures[entityIndices[eId]] = aux;
            entityIndices[aux.eId] = entityIndices[eId];
            entityIndices[eId] = lastIndex;
        }
        markedBegin++;
    }

    void removeEntity(Entity eId)
    {
        components.removeEntity(eId);
        previousIds.push(eId);
    }

    bool entityExists(Entity eId) const
    {
        return entityIndices[eId] < signatureCount;
    }

    template<typename Component>
    ComponentView<Component> iterateComponents()
    {
        ComponentVector<Component>& cvec = components.template getComponentVector<Component>();
        return cvec.iterateComponents();
    }

    template<typename Component>
    const ComponentView<Component> iterateComponents() const
    {
        const ComponentVector<Component>& cvec = components.template getComponentVector<Component>();
        return cvec.iterateComponents();
    }

    template<typename ... Components>
    SignatureView<TSignature<_ComponentList, Components ...>, _ComponentList> iterateEntities()
    {
        SignatureView<TSignature<_ComponentList, Components ...>, _ComponentList> retval(&entitySignatures[0], &entitySignatures[signatureCount]);
        return retval;
    }

    template<typename ... Components>
    const SignatureView<TSignature<_ComponentList, Components ...>, _ComponentList> iterateEntities() const
    {
        const SignatureView<TSignature<_ComponentList, Components ...>, _ComponentList> retval(&entitySignatures[0], &entitySignatures[signatureCount]);
        return retval;
    }

private:

    IDStack previousIds;
    static constexpr u32 signatureCapacity = KAMSKI_MAX_ENTITY_COUNT;
    u32 entityIndices[signatureCapacity];
    Signature entitySignatures[signatureCapacity];
    u32 signatureCount;
    u32 markedBegin;

    _ComponentList components;
    Entity nextEntity;
};
