/**
 * @file: layout.cpp 
 * Layout algorithms implementation file
 * Layout library, 2d graph placement of graphs in ShowGraph tool.
 * Copyright (C) 2009  Boris Shurygin
 */

#include "layout_iface.h"

/**
 * Compare orders of nodes
 */
bool compareOrders( AuxNode* node1,
                    AuxNode* node2)
{
    return ( node1->order() < node2->order());
}

/**
 * compare center coordinates of group
 */
bool compareGroups( NodeGroup* g1,
                    NodeGroup* g2)
{
    /*
     * center = g.left + width / 2 = g.left + ( g.right - g.left) / 2 = ( g.left + g.right)/2
     *
     *                g1.center < g2.center 
     * (g1.left + g1.right) / 2 < (g2.left + g2.right) / 2
     *       g1.left + g1.right < g2.left + g2.right
     */
    return ( g1->left() + g1->right() < g2->left() + g2->right());
}

/**
 * Sort nodes in a level with respect to their order
 */
void Level::sortNodesByOrder()
{
    qSort( node_list.begin(), node_list.end(), compareOrders);
}

/**
 * Arranges nodes using group merge algorithm.
 * NodeGroup is a group of nodes which interleave if we apply baricentric heuristic directly.
 * These nodes are placed within group borders. If two groups interleave they are merged.
 * Arrangement is performed iteratively starting with groups that have one node each.
 */
void Level::arrangeNodes( GraphDir dir, bool commit_placement, bool first_pass)
{
    QList< NodeGroup *> list;
    foreach ( AuxNode* node, node_list)
    {
        if ( node->isForPlacement())
        {
            NodeGroup* group = new NodeGroup( node, dir, first_pass);
            list.push_back( group);
        }
    }
    /** Sort groups with respect to their coordinates */
    qSort( list.begin(), list.end(), compareGroups);
    
    QLinkedList< NodeGroup *> groups;
    
    foreach( NodeGroup *group, list)
    {
        groups.push_back( group);
    }
    QLinkedList< NodeGroup *>::iterator it = groups.begin();

    /**
     * For each group
     */
    while( groups.count())
    {
        /*
         * 1. Look at the group to the right and left and see they interleave
         *    if they do -> merge groups and repeat
         */
        NodeGroup* grp = *it;
        QLinkedList< NodeGroup *>::iterator it_right = it;
        it_right++;
        bool no_merge = true; 

        /** Group to the left */
        if ( it != groups.begin())
        {
            QLinkedList< NodeGroup *>::iterator it_left = it;
            it_left--;
            NodeGroup* left_grp = *it_left;
            if ( grp->interleaves( left_grp))
            {
                groups.erase( it_left);
                grp->merge( left_grp);
                no_merge = false;
                delete ( left_grp);
            }
        }
        /** Group to the right */
        if ( it_right != groups.end())
        {
            NodeGroup* right_grp = *it_right;
            if ( grp->interleaves( right_grp))
            {
                groups.erase( it_right);
                grp->merge( right_grp);
                no_merge = false;
                delete ( right_grp);
            }    
        }
        /** Proceed to the next group */
        if ( no_merge)
            it++;
        /** End loop if we have processed all groups and merged everything we could */
        if ( it == groups.end())
            break;
    }

    if ( commit_placement)
    {
        /** Assign coordinates to nodes */
        for ( it = groups.begin(); it != groups.end(); it++)
        {
            NodeGroup *grp = *it;
            grp->placeNodesFinal( dir);
            delete grp;
        }
    } else 
    {
        /** Assign coordinates to nodes */
        for ( it = groups.begin(); it != groups.end(); it++)
        {
            NodeGroup *grp = *it;
            grp->placeNodes();
            delete grp;
        }
    }
}

/**
 * Mark nodes that are reachable in direction GRAPH_DIR_DOWN from given node 
 */
GraphNum
AuxGraph::markReachableDown( AuxNode *n,
                             Marker m)
{
    GraphNum marked = 1;
    QStack< AuxNode *> trav;
    trav.push( n);
    n->mark( m);
    while ( !trav.isEmpty())
	{
		AuxNode *n = trav.pop();
		AuxEdge *e;
		
		foreachSucc( e, n)
		{
			AuxNode* succ = e->succ();
			if ( succ->mark( m))
			{
				marked++;
				trav.push( succ);
			}
		}
	}
    return marked;
}

/**
 * Find enter nodes
 */
QStack< AuxGraph::DfsStepInfo *>
AuxGraph::findEnterNodes()
{
	QStack< DfsStepInfo *> stack;
    Marker m = newMarker();
	GraphNum marked = 0;
	QStack< AuxNode *> trav;

    /** Find nodes that have no predecessors */
    for ( AuxNode *n = firstNode();
          isNotNullP( n);
          n = n->nextNode())
    {
        if ( isNullP( n->firstPred()))
        {
            n->mark( m);
			marked++;
			stack.push( new DfsStepInfo( n));
			trav.push( n);
		} else
		{
			bool has_valid_pred = false;
			AuxEdge* e = n->firstPred();
			
			while ( isNotNullP( e))
			{
				if ( areNotEqP( e->pred(),n))
				{
					has_valid_pred = true;
					break;
				}
				e = e->nextPred();
			}
			if ( !has_valid_pred)
			{
				n->mark( m);
				marked++;
				stack.push( new DfsStepInfo( n));
				trav.push( n);
			}
		}
    }
	/** Mark nodes from enters */
	while ( !trav.isEmpty())
	{
		AuxNode *n = trav.pop();
		AuxEdge *e;
		
		ForEdges( n, e, Succ)
		{
			AuxNode* succ = e->succ();
			if ( succ->mark( m))
			{
				marked++;
				trav.push( succ);
			}
		}
	}
	/** Check if we're done */
	if ( marked == nodeCount())
	{
		freeMarker( m);
		return stack;
	}
	
    /** Begin traverse from exits */
    QStack< DfsStepInfo *> rev_trav;
    Marker visited = newMarker();
	
    /* Fill stack with nodes that have no successors */
    for ( AuxNode *n = firstNode();
          isNotNullP( n);
          n = n->nextNode())
    {
		bool has_valid_succ = false;
		
		if ( n->isMarked( m))
			continue;
		
		AuxEdge* e = n->firstSucc();
		
		while ( isNotNullP( e))
		{
			if ( !e->succ()->isMarked( m) && areNotEqP( e->succ(),n))
			{
				has_valid_succ = true;
				break;
			}
			e = e->nextSucc();
		}
		if ( !has_valid_succ)
		{
			n->mark( m);
            n->mark( visited);
            rev_trav.push( new DfsStepInfo( n, GRAPH_DIR_UP));
		}
    }

	/** Upward pass */
    while( !rev_trav.isEmpty())
    {
        DfsStepInfo *info = rev_trav.top();
        AuxNode *node = info->node;
        AuxEdge *edge = info->edge;
        
        if ( isNotNullP( edge)) // Add predecessor to stack
        {
            AuxNode* pred_node = edge->pred();
            info->edge = edge->nextPred();
            
            if ( !pred_node->isMarked( m)
                 && pred_node->isMarked( visited))
            {
                //Backedge in reverse traversal terms. Consider edge's predecessor as enter node
                stack.push( new DfsStepInfo( pred_node));
            }
            if ( pred_node->mark( visited))
                 rev_trav.push( new DfsStepInfo( pred_node, GRAPH_DIR_UP));
        } else // We're done with this node
        {
            node->mark( m);
            marked++;
            delete info;
            rev_trav.pop();
        }
    }
    freeMarker( visited);
	
    /** Check if we're done */
	if ( marked == nodeCount())
	{
		freeMarker( m);
		return stack;
	}

    /** 
     *  If we didn't find all the nodes on previous passes then
     *  they must be in infinite loops without a head so let's break these loops
     *  r a n d o m l y
     */
    for ( AuxNode *n = firstNode();
          isNotNullP( n);
          n = n->nextNode())
    {
        if ( !n->isMarked( m))
        {
            stack.push( new DfsStepInfo( n));
            marked+=markReachableDown( n, m);
        }
    }
    assertd( marked == nodeCount());// FIXME: node count needs updating on node delete
    freeMarker( m);
    return stack;
}

/**
 * Perform edge classification
 */
void AuxGraph::classifyEdges()
{
    Marker m = newMarker(); // Marker for visiting nodes
    Marker doneMarker = newMarker(); // Marker for nodes that are finished
    AuxEdge* e;

    foreachEdge( e, this)
    {
        e->setUnknown();   
    }

	/* Fill the traverse stack with enter nodes */
	QStack< DfsStepInfo *> stack = findEnterNodes();
    
    /* Walk graph with marker and perform classification */
    while( !stack.isEmpty())
    {
        DfsStepInfo *info = stack.top();
        AuxNode *node = info->node;
        AuxEdge *edge = info->edge;
        
        if ( isNotNullP( edge)) // Add successor to stack
        {
            AuxNode* succ_node = edge->succ();
            info->edge = edge->nextSucc();
            
            if ( !succ_node->isMarked( doneMarker)
                 && succ_node->isMarked( m))
            {
                edge->setBack();
                AuxNode *pred = edge->pred();
                while ( pred->isEdgeLabel())
                {
                    pred->firstPred()->setBack();
                    pred = pred->firstPred()->pred();
                }
            }
            if ( succ_node->mark( m))
                 stack.push( new DfsStepInfo( succ_node));
        } else // We're done with this node
        {
            node->mark( doneMarker);
            delete info;
            stack.pop();
        }
    }

    freeMarker( m);
    freeMarker( doneMarker);
    return;
}

/**
 * Ranking of nodes. Level distribution of nodes. Marks tree edges.
 */
Numeration AuxGraph::rankNodes()
{
    QVector< int> pred_nums( nodeCount());
    QStack< AuxNode *> stack; // Node stack
    
    invalidateRanking();

    Numeration own = newNum();
    GraphNum i = 0;
    max_rank = 0;
    /**
     *  Set numbers to nodes and count predecessors of each node.
     *  predecessors include inverted edges 
     */
    AuxNode *n;
    foreachNode( n, this)
    {
        int pred_num = 0;
        n->setNumber( own, i);
        AuxEdge* e; 
        foreachPred( e, n)
        {
            if ( e->pred() == e->succ())
                continue;

            if ( !e->isInverted())
                pred_num++;
        }
        foreachSucc( e, n)
        {
            if ( e->pred() == e->succ())
                continue;

            if ( e->isInverted())
                pred_num++;
        }
        pred_nums[ i] = pred_num;
        i++;
    }
    /* Fill ranking and ordering numerations by walking the nodes */
    /* Add nodes with no preds to stack */
    foreachNode( n, this)
    {
        if ( pred_nums[ n->number( own)] == 0)
        {
            stack.push( n);
        }
    }
    while( !stack.isEmpty())
    {
        AuxNode* n = stack.pop();
        AuxEdge* e;
        GraphNum rank = 0;

        /* Propagation part */
        foreachPred( e, n)
        {
            if ( e->pred() == e->succ())
                continue;

            if ( !e->isInverted())
            {
                if ( rank < e->pred()->number( ranking) + 1)
                {
                    rank = e->pred()->number( ranking) + 1;
                }
            }
        }
        foreachSucc( e, n)
        {
            if ( e->pred() == e->succ())
                continue;

            if ( e->isInverted())
            {
                if ( rank < e->succ()->number( ranking) + 1)
                {
                    rank = e->succ()->number( ranking) + 1;
                }
            }
        }

        if ( rank > max_rank)
            max_rank = rank;

        n->setNumber( ranking, rank);
#ifdef _DEBUG
        out( "%llu node rank is %u", n->id(), rank);
#endif
        n->setY( rank * RANK_SPACING);

        /* Traversal continuation */
        foreachSucc( e, n)
        {
            if ( e->pred() == e->succ())
                continue;

            if ( !e->isInverted())
            {
                AuxNode* succ = e->succ();
                pred_nums[ succ->number( own)] =
                    pred_nums[ succ->number( own)] - 1;
                
                if ( pred_nums[ succ->number( own)] == 0)
                {
                    stack.push( succ);
                }
            }  
        }
        foreachPred( e, n)
        {
            if ( e->pred() == e->succ())
                continue;
            if ( e->isInverted())
            {
                AuxNode* succ = e->pred();
                pred_nums[ succ->number( own)] =
                    pred_nums[ succ->number( own)] - 1;
                if ( pred_nums[ succ->number( own)] == 0)
                {
                    stack.push( succ);
                }
            }
        }
    }
    freeNum( own);

    /** Fill levels */
    initLevels( maxRank());
    foreachNode( n, this)
    {
        Rank rank = n->number( ranking);
        if ( rank == NUMBER_NO_NUM)
        {
            rank = 0;
            assert( 0); // Shouldn't be here. Means ranking did not cover all nodes
        } 
        levels[ rank]->add( n);
    }
    /** Create edge control nodes */
    AuxEdge* e;
    foreachEdge( e, this)
    {
        AuxNode* pred;
        AuxNode* succ;
        
        if ( e->pred() == e->succ())
            continue;

        if ( e->isInverted())
        {
            pred = e->succ();
            succ = e->pred();
        } else
        {
            pred = e->pred();
            succ = e->succ();
        }

        Rank pred_rank = pred->number( ranking);
        Rank succ_rank = succ->number( ranking);
        if ( pred_rank == NUMBER_NO_NUM)
        {
            pred_rank = 0;
        } 
        if ( succ_rank == NUMBER_NO_NUM)
        {
            succ_rank = pred_rank + 1;
        } 
        Rank curr_rank = pred_rank + 1;
        AuxEdge *curr_edge = e;
        while ( curr_rank != succ_rank)
        {
            AuxNode *node = curr_edge->insertNode();
            if ( e->isInverted())
            {
                curr_edge = node->firstPred();
            } else
            {
                curr_edge = node->firstSucc();
            }
            node->firstSucc()->setType( e->type());
            node->setType( AUX_EDGE_CONTROL);
            node->setY( pred->modelY() + RANK_SPACING);
            levels[ curr_rank]->add( node);
            node->setNumber( ranking, curr_rank);
            pred = node;
            curr_rank++;
        }
    }
    validateRanking();
#ifdef _DEBUG
    //debugPrint();
#endif
    return ranking; 
}

/**
 * Perform layout
 */
void AuxGraph::doLayout()
{
    /**
     * 0. Remove all edge controls
     * FIXME: This is a stub. we should not delete controls,
     *        instead we should reuse them and create new ones only if necessary
     */
    for ( AuxNode* n = firstNode();
          isNotNullP( n);
          )
    {
        AuxNode *next = n->nextNode();
        if ( n->isEdgeControl())
        {
            delete n;
        }
        n = next;
    }

    /** 1. Perfrom edge classification */
    classifyEdges();
    
    /** 2. Rank nodes */
    rankNodes();

    /** 3. Adjust levels vertically */
    adjustVerticalLevels();

    /** 4. Perform edge crossings minimization */
    reduceCrossings();

    /** 5. Perform horizontal arrangement of nodes */
    arrangeHorizontally();

    /** 6. Move edge controls to enchance the picture readability */
}

/**
 * Assign order to nodes by numbering in a DFS traversal
 */
void AuxGraph::orderNodesByDFS()
{
    /** Structure used for dfs traversal */
    struct DfsStepInfo
    {
        AuxNode *node; // Node in consideration
        AuxEdge *edge; // Next edge

        /* Constructor */
        DfsStepInfo( AuxNode *n)
        {
            node = n;
            edge = n->firstSucc();
        }
    };
    
    Marker m = newMarker(); // Marker for visiting nodes
    QStack< DfsStepInfo *> stack;
    GraphNum num = 0;
    
    /* Fill stack with nodes that have no predecessors */
    for ( AuxNode *n = firstNode();
          isNotNullP( n);
          n = n->nextNode())
    {
        if ( isNullP( n->firstPred()) && !n->isMarked( m))
        {
            n->setOrder( num++);
            stack.push( new DfsStepInfo( n));

            /* Walk graph with marker and perform classification */
            while( !stack.isEmpty())
            {
                DfsStepInfo *info = stack.top();
                AuxNode *node = info->node;
                AuxEdge *edge = info->edge;
                
                if ( isNotNullP( edge)) // Add successor to stack
                {
                    AuxNode* succ_node = edge->succ();
                    info->edge = edge->nextSucc();
                    
                    if ( succ_node->mark( m))
                    {
                        stack.push( new DfsStepInfo( succ_node));
                        succ_node->setOrder( num++);
                    }
                } else // We're done with this node
                {
                    delete info;
                    stack.pop();
                }
            }
        }
    }
    freeMarker( m);
}

/**
 * Arrange ranks vertically
 */
void AuxGraph::adjustVerticalLevels()
{
    qreal y = 0;
    qreal prev_height = 0;
    for ( int i = 0; i < levels.size(); i++)
    {
        Level* level = levels[ i];
        y += (prev_height + level->height() + RANK_SPACING) / 2;
        level->setY( y );
        prev_height = level->height();
    }
}

/**
 * Try to reduce number of edge crossings between levels
 */
void AuxGraph::reduceCrossings()
{
    /** Perform numeration and sort nodes to avoid tree edges crossings */
    orderNodesByDFS();

    for ( int i = 0; i < levels.size(); i++)
    {
        Level* level = levels[ i];
        level->sortNodesByOrder();
    }
}

/**
 * Assign X coordinates to the nodes
 */
void AuxGraph::arrangeHorizontally()
{
    /* Descending pass */
    for ( int i = 0; i < levels.size(); i++)
    {
        levels[ i]->arrangeNodes( GRAPH_DIR_DOWN, false, true);
    }
    
    /* Ascending pass */
    for ( int i = levels.size() - 1; i >= 0; i--)
    {
        levels[ i]->arrangeNodes( GRAPH_DIR_UP, false, false);
    }
    /* Final pass */
    for ( int i = 0; i < levels.size(); i++)
    {
        levels[ i]->arrangeNodes( GRAPH_DIR_DOWN, true, false);
    }
}