with open(r'b:\ADS Project\backend\src\api\server.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

target = '''        if (rotType == "LL") {
            // Left-Left (Single Right Rotation): insert 30, 20, 10
            tree.insert("INC-30", makeInc("INC-30"), &trace);
            tree.insert("INC-20", makeInc("INC-20"), &trace);
            tree.insert("INC-10", makeInc("INC-10"), &trace);
        } else if (rotType == "RR") {
            // Right-Right (Single Left Rotation): insert 10, 20, 30
            tree.insert("INC-10", makeInc("INC-10"), &trace);
            tree.insert("INC-20", makeInc("INC-20"), &trace);
            tree.insert("INC-30", makeInc("INC-30"), &trace);
        } else if (rotType == "LR") {
            // Left-Right (Double Rotation): insert 30, 10, 20
            tree.insert("INC-30", makeInc("INC-30"), &trace);
            tree.insert("INC-10", makeInc("INC-10"), &trace);
            tree.insert("INC-20", makeInc("INC-20"), &trace);
        } else if (rotType == "RL") {
            // Right-Left (Double Rotation): insert 10, 30, 20
            tree.insert("INC-10", makeInc("INC-10"), &trace);
            tree.insert("INC-30", makeInc("INC-30"), &trace);
            tree.insert("INC-20", makeInc("INC-20"), &trace);
        } else if (rotType == "BALANCED" || rotType == "NONE") {
            // Balanced Insertion Sequence (No Rotations Required): insert 20, 10, 30
            tree.insert("INC-20", makeInc("INC-20"), &trace);
            tree.insert("INC-10", makeInc("INC-10"), &trace);
            tree.insert("INC-30", makeInc("INC-30"), &trace);
        } else if (action == "inorder") {
            tree.insert("INC-20", makeInc("INC-20"));
            tree.insert("INC-10", makeInc("INC-10"));
            tree.insert("INC-30", makeInc("INC-30"));
            tree.inorderTraversal(&trace);
        }'''

replacement = '''        if (action == "inorder") {
            tree.insert("INC-20", makeInc("INC-20"));
            tree.insert("INC-10", makeInc("INC-10"));
            tree.insert("INC-30", makeInc("INC-30"));
            TraceEvent initEv;
            initEv.algorithm = "AVLTree";
            initEv.operation = "initial_state";
            initEv.entityId = "TREE_READY";
            initEv.explanation = "Initial state: balanced AVL tree (INC-20, INC-10, INC-30) ready for inorder traversal.";
            initEv.metadata = {
                {"empty", false},
                {"treeSnapshot", tree.toJsonSnapshot()["root"]}
            };
            trace.addEvent(std::move(initEv));
            tree.inorderTraversal(&trace);
        } else {
            TraceEvent initEv;
            initEv.algorithm = "AVLTree";
            initEv.operation = "initial_state";
            initEv.entityId = "EMPTY_TREE";
            initEv.explanation = "Initial state: AVL tree is empty before inserting keys.";
            initEv.metadata = {
                {"empty", true},
                {"treeSnapshot", nullptr}
            };
            trace.addEvent(std::move(initEv));

            if (rotType == "LL") {
                // Left-Left (Single Right Rotation): insert 30, 20, 10
                tree.insert("INC-30", makeInc("INC-30"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
                tree.insert("INC-10", makeInc("INC-10"), &trace);
            } else if (rotType == "RR") {
                // Right-Right (Single Left Rotation): insert 10, 20, 30
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
                tree.insert("INC-30", makeInc("INC-30"), &trace);
            } else if (rotType == "LR") {
                // Left-Right (Double Rotation): insert 30, 10, 20
                tree.insert("INC-30", makeInc("INC-30"), &trace);
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
            } else if (rotType == "RL") {
                // Right-Left (Double Rotation): insert 10, 30, 20
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-30", makeInc("INC-30"), &trace);
                tree.insert("INC-20", makeInc("INC-20"), &trace);
            } else if (rotType == "BALANCED" || rotType == "NONE") {
                // Balanced Insertion Sequence (No Rotations Required): insert 20, 10, 30
                tree.insert("INC-20", makeInc("INC-20"), &trace);
                tree.insert("INC-10", makeInc("INC-10"), &trace);
                tree.insert("INC-30", makeInc("INC-30"), &trace);
            }
        }'''

# Normalize line endings for replacement
norm_content = content.replace('\r\n', '\n')
norm_target = target.replace('\r\n', '\n')
norm_replacement = replacement.replace('\r\n', '\n')

assert norm_target in norm_content, "target block not found in server.cpp"
norm_content = norm_content.replace(norm_target, norm_replacement, 1)

with open(r'b:\ADS Project\backend\src\api\server.cpp', 'w', encoding='utf-8') as f:
    f.write(norm_content)

print("server.cpp successfully updated!")
