# Script to delete tags from remote and local if they failed to build


# import variables from versions.ps1
. ./versions.ps1


# Get version from cli argument
$modVersion = $args[0]

# Check if the input is empty
if ($modVersion -eq "") {
    Write-Host "No version given! (example: 0.2.12)"
    exit
}

# Check if version is in format x.x.x and have no other characters 
if ($modVersion -notmatch '^\d+\.\d+\.\d+$') {
    Write-Host "Version is not in format x.x.x (example: 0.2.12) or have other characters! it's the first argument"
    exit
}

# Print version
Write-Host "Gonna do version: $modVersion"


# Create new tags array
$new_tags = @()

Write-Host "Tags are:"
Write-Host "-----------------"
# Loop through versions and tags 1:1
for ($i = 0; $i -lt $versions.Length; $i++) {
    # Get version
    $v = $versions[$i]
    
    # Print tag
    Write-Host "v$modVersion-bs-$v"

    # v0.2.12-bs-1.24.0
    $new_tags += "v$modVersion-bs-$v"
}
Write-Host "-----------------"


  
# Ask if we should continue
Write-Host "Delete these tags? (y/n)"
$continue = Read-Host

# If not then exit
if ($continue -ne "y") {
    exit
}

Write-Host "Fetching tags from remote..."

# Fetch tags from remote
git fetch --tags
Write-Host "Done!"


# Loop through tags
for ($i = 0; $i -lt $new_tags.Length; $i++) {
    # Get tag
    $tag = $new_tags[$i]
   

    # Check if the tag already exists, if it does then skip it
    $tagExists = git tag | Select-String -Pattern $tag
    if (!$tagExists) {
        Write-Host "Tag $tag does not exist! skipping..."
        continue
    }

    # Remove tag from remote
    Write-Host "Removing tag $tag from remote..."
    # Delete tag from remote
    git push origin --delete $tag
    # Delete tag from local
    git tag -d $tag
    Write-Host "Done!"

    
    Write-Host "Remove tag $tag from local and remote..." 
}




Write-Host "Done! All tags are deleted! Yaay! 🎉"