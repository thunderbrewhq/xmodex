#ifndef XMODEX_LINKED_LIST_HPP
#define XMODEX_LINKED_LIST_HPP

#include <cstdint>

namespace FMOD {

class LinkedListNode {
        friend class SortedLinkedListNode;

    private:
        LinkedListNode* mNodeNext;
        LinkedListNode* mNodePrev;
        void* mNodeData;

    public:
        inline LinkedListNode() {
            this->initNode();
        }

        inline LinkedListNode* getNext() {
            return this->mNodeNext;
        }

        inline LinkedListNode* getPrev() {
            return this->mNodePrev;
        }

        inline void setData(void* data) {
            this->mNodeData = data;
        }

        inline void* getData() {
            return this->mNodeData;
        }

        inline void initNode() {
            this->mNodeNext = this;
            this->mNodePrev = this;
            this->mNodeData = nullptr;
        }

        inline void removeNode() {
            this->mNodePrev->mNodeNext = this->mNodeNext;
            this->mNodeNext->mNodePrev = this->mNodePrev;
            this->mNodePrev = this;
            this->mNodeNext = this;
            this->mNodeData = nullptr;
        }

        void addAfter(LinkedListNode* node) {
            this->mNodeNext = node->mNodeNext;
            this->mNodePrev = node;
            this->mNodeNext->mNodePrev = this;
            this->mNodePrev->mNodeNext = this;
        }

        void addBefore(LinkedListNode* node) {
            this->mNodePrev = node->mNodePrev;
            this->mNodeNext = node;
            this->mNodeNext->mNodePrev = this;
            this->mNodePrev->mNodeNext = this;
        }

        bool exists(LinkedListNode* node) const {
            auto current = this->mNodeNext;
            do {
                if (current == node) {
                    return true;
                }
                current = current->mNodeNext;
            } while (current != this);
            return false;
        }

        inline bool isEmpty() const {
            return this->mNodeNext == this && this->mNodePrev == this;
        }

        int32_t count() const {
            auto current = this->mNodeNext;
            int32_t i = 0;
            while (current != this) {
                current = current->mNodeNext;
                i++;
            }
            return i;
        }

        LinkedListNode* getNodeByIndex(int32_t i) const {
            if (i < 0) {
                return nullptr;
            }
            auto current = this->mNodeNext;
            while (i > 0) {
                current = current->mNodeNext;
                if (current == this) {
                    return nullptr;
                }
                i--;
            }
            return current;
        }

        int32_t getNodeIndex(LinkedListNode* node) const {
            auto current = this->mNodeNext;
            for (int32_t i = 0; current != this; i++) {
                if (current == node) {
                    return i;
                }
                current = current->mNodeNext;
            }
            return -1;
        }
};

class SortedLinkedListNode : public LinkedListNode {
    private:
        uint32_t mNodePriority;

    public:
        SortedLinkedListNode() {
            initNode();
        }

        inline void initNode() {
            this->mNodePriority = 0xFFFFFFFF;
            this->mNodeNext = this;
            this->mNodePrev = this;
            this->mNodeData = nullptr;
        }

        inline void removeNode() {
            this->mNodePrev->mNodeNext = this->mNodeNext;
            this->mNodeNext->mNodePrev = this->mNodePrev;
            this->mNodePrev = this;
            this->mNodeNext = this;
            this->mNodePriority = 0xFFFFFFFF;
            this->mNodeData = nullptr;
        }

        void addAt(SortedLinkedListNode* node, uint32_t priority) {
            auto current = static_cast<SortedLinkedListNode*>(node->mNodeNext);
            do {
                if (priority < current->mNodePriority) {
                    this->mNodePriority = priority;
                    this->addBefore(current);
                    return;
                }
                current = static_cast<SortedLinkedListNode*>(current->mNodeNext);
            } while (current->mNodePrev != node);
        }
};

} // namespace FMOD

#endif
