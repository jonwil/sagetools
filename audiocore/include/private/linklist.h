/*
**  sagetools
**  Copyright 2026 Jonathan Wilson
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LINKLIST_H
#define LINKLIST_H

class ListNode
{
private:
    ListNode* pnext;

public:
    void SetNext(ListNode* pnode) { pnext = pnode; }
    ListNode* GetNext() const { return pnext; }
};

class ListQueue
{
private:
    ListNode* phead;
    ListNode* ptail;
    int entries;

public:
    ListQueue()
    {
        phead = 0;
        ptail = 0;
        entries = 0;
    }

    void Reset()
    {
        phead = 0;
        ptail = 0;
        entries = 0;
    }

    bool IsEmpty() { return phead ? false : true; }
    ListNode* GetHead() const { return phead; }
    ListNode* GetTail() const { return ptail; }
    int GetEntries() const { return entries; }

    void Push(ListNode* pnode)
    {
        pnode->SetNext(phead);

        if (!ptail)
        {
            ptail = pnode;
        }

        phead = pnode;
        entries++;
    }

    void PushTail(ListNode* pnode)
    {
        pnode->SetNext(0);

        if (!phead)
        {
            phead = pnode;
        }
        else
        {
            ptail->SetNext(pnode);
        }

        ptail = pnode;
        entries++;
    }

    ListNode* Pop()
    {
        ListNode* pnode;
        pnode = phead;

        if (phead)
        {
            phead = phead->GetNext();

            if (!phead)
            {
                ptail = 0;
            }

            entries--;
        }

        return pnode;
    }
};

#endif
