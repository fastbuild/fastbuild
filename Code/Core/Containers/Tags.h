// Tags.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/IOStream.h"

#define TAG_DELIMITER               "="
#define TAG_NOT_OPERATOR            "!"
#define TAG_NOT_OPERATOR_DIR_PREFIX "not_"

// Tag
//------------------------------------------------------------------------------
class Tag
{
    public:
        inline explicit Tag() : m_KeyInverted( false ), m_ValueInverted( false ) {}
        inline virtual ~Tag() = default;

        inline bool  GetKeyInverted() const;
        inline const AString & GetKey() const;
        inline void  SetKey( const AString & keyString );
        inline bool  GetValueInverted() const;
        inline const AString & GetValue() const;
        inline void  SetValue( const AString & valueString );
        inline void  ToString( AString & string ) const;

        inline bool Matches( const Tag & tag ) const;
        inline bool ContainsValidDirChars( AString & errorMsg ) const;
        inline void ToDirNameArray( Array< AString > & dirNameArray ) const;

        inline void Read( IOStream & stream );
        inline void Write( IOStream & stream ) const;

        inline bool operator == ( const Tag & other ) const;
        inline bool operator != ( const Tag & other ) const { return !(*this == other ); }
        inline bool operator < ( const Tag & other ) const;

    private:
        inline void SetKeyParts( const bool keyInverted, const char * key );
        inline void SetValueParts( const bool valueInverted, const char * value );

        AString m_Key;
        bool    m_KeyInverted;
        AString m_Value;
        bool    m_ValueInverted;
};

// Tags
//------------------------------------------------------------------------------
class Tags
{
    public:
        inline Tags();
        inline virtual ~Tags();

        inline size_t GetSize() const;
        inline bool  IsValid() const;
        inline bool  IsEmpty() const;
        inline void  SetValid( const bool isValid );
        inline Tag & Get( size_t index );
        inline const Tag & Get( size_t index ) const;
        inline Tag & Top();
        inline const Tag &  Top() const;
        inline const AString & GetWorkerName() const;
        inline void SetWorkerName( const AString & workerName );
        inline void SetCapacity( const size_t capacity );
        inline void Sort();
        inline void Read( IOStream & stream );
        inline void Write( IOStream & stream ) const;
        inline bool FindKey( const char * key, size_t & foundIndex ) const;
        inline bool MatchesAll( const Tags & searchTags ) const;
        inline void GetNoWorkerFoundError(
            const bool distributableJob,
            const AString & nodeName,
            AString & noWorkerError ) const;
        inline void ParseAndAddTag( const AString & tagString );
        inline void ParseAndAddTags( const Array< AString > & tagStrings );
        inline void Append( const Tag & tag );
        inline void EraseIndex( size_t index );
        inline void Clear();
        inline void GetChanges( const Tags & currentTags,
            Tags & removedTags, Tags & addedTags ) const;
        inline bool ApplyChanges( const Tags & removedTags, const Tags & addedTags );
        inline bool ContainsValidDirChars( AString & errorMsg ) const;
        inline void ToStringArray( Array< AString > & stringArray ) const;
        inline void ToArgsString( AString & string ) const;

        inline bool operator == ( const Tags & other ) const;
        inline bool operator != ( const Tags & other ) const { return !(*this == other ); }

    private:
        AString      m_WorkerName;
        Array< Tag > m_Tags;
        bool         m_IsValid;
};

// GetKeyInverted (Tag)
//------------------------------------------------------------------------------
bool Tag::GetKeyInverted() const
{
    return m_KeyInverted;
}

// GetKey (Tag)
//------------------------------------------------------------------------------
const AString & Tag::GetKey() const
{
    return m_Key;
}

// SetKey (Tag)
//------------------------------------------------------------------------------
void Tag::SetKey( const AString & keyString )
{
    if ( keyString.BeginsWith( TAG_NOT_OPERATOR ) )
    {
        SetKeyParts( true, keyString.Get() + 1 );
    }
    else
    {
        SetKeyParts( false, keyString.Get() );
    }
}

// SetKeyParts (Tag)
//------------------------------------------------------------------------------
void Tag::SetKeyParts( const bool keyInverted, const char * key )
{
    m_KeyInverted = keyInverted;
    m_Key = key;
}

// GetValueInverted (Tag)
//------------------------------------------------------------------------------
bool Tag::GetValueInverted() const
{
    return m_ValueInverted;
}

// GetValue (Tag)
//------------------------------------------------------------------------------
const AString & Tag::GetValue() const
{
    return m_Value;
}

// SetValue (Tag)
//------------------------------------------------------------------------------
void Tag::SetValue( const AString & valueString )
{
    if ( valueString.BeginsWith( TAG_NOT_OPERATOR ) )
    {
        SetValueParts( true, valueString.Get() + 1 );
    }
    else
    {
        SetValueParts( false, valueString.Get() );
    }
}

// SetValueParts (Tag)
//------------------------------------------------------------------------------
void Tag::SetValueParts( const bool valueInverted, const char * value )
{
    m_ValueInverted = valueInverted;
    m_Value = value;
}

// ToString (Tag)
//------------------------------------------------------------------------------
void Tag::ToString( AString & string ) const
{
    if ( m_KeyInverted )
    {
        string += TAG_NOT_OPERATOR;
    }
    string.Append( m_Key );
    if ( !m_Value.IsEmpty() )
    {
        string += TAG_DELIMITER;
        if ( m_ValueInverted )
        {
            string += TAG_NOT_OPERATOR;
        }
        string += m_Value;
    }
}

// Matches (Tag)
//------------------------------------------------------------------------------
bool Tag::Matches( const Tag & tag ) const
{
    const bool checkValue = !tag.GetValue().IsEmpty();
    bool keyMatches = false;
    bool valueMatches = false;
    // wildcard match the key and the value,
    // since tag could have non-empty m_Key but empty m_Value
    keyMatches = GetKey().Matches( tag.GetKey().Get() );
    if ( keyMatches )
    {
        if ( checkValue )
        {
            valueMatches = GetValue().Matches( tag.GetValue().Get() );
        }
    }
    if ( tag.GetKeyInverted() )
    {
        if ( checkValue )
        {
            // apply the inversion to the value
            valueMatches = !valueMatches;
        }
        else
        {
            // apply the inversion to the key
            keyMatches = !keyMatches;
        }
    }
    if ( checkValue && tag.GetValueInverted() )
    {
        valueMatches = !valueMatches;
    }
    if ( keyMatches )
    {
        if ( GetKeyInverted() )
        {
            if ( GetValue().IsEmpty() )
            {
                // apply the inversion to the key
                keyMatches = !keyMatches;
            }
            else if ( checkValue )
            {
                // apply the inversion to the value
                valueMatches = !valueMatches;
            }
        }
        if ( checkValue && GetValueInverted() )
        {
            valueMatches = !valueMatches;
        }
    }
    bool matches = false;
    if ( checkValue )
    {
        matches = keyMatches && valueMatches;
    }
    else
    {
        matches = keyMatches;
    }
    return matches;
}

// ContainsValidDirChars (Tag)
//------------------------------------------------------------------------------
bool Tag::ContainsValidDirChars( AString & errorMsg ) const
{
    // always call both contains methods here, since want to show user all errors at once
    AStackString<> keyErrorMsg;
    const bool keyValid = FileIO::ContainsValidDirChars( m_Key, keyErrorMsg );
    AStackString<> valueErrorMsg;
    const bool valueValid = FileIO::ContainsValidDirChars( m_Value, valueErrorMsg );
    const bool valid = keyValid && valueValid;
    if ( !valid )
    {
        AStackString<> tagString;
        ToString( tagString );
        errorMsg += "Tag ";
        errorMsg += tagString;
        errorMsg += " contains invalid chars:\n";
        AStackString<> fourSpaces( "    " );
        if ( !keyErrorMsg.IsEmpty() )
        {
            errorMsg += fourSpaces;  // indent 4 spaces
            errorMsg += keyErrorMsg;
            errorMsg += "\n";
        }
        if ( !valueErrorMsg.IsEmpty() )
        {
            errorMsg += fourSpaces;  // indent 4 spaces
            errorMsg += valueErrorMsg;
            errorMsg += "\n";
        }
    }
    return valid;
}

// ToDirNameArray (Tag)
//------------------------------------------------------------------------------
void Tag::ToDirNameArray( Array< AString > & dirNameArray ) const
{
    dirNameArray.Clear();  // clear array, so we can populate it
    AStackString<> dirName;
    // for an inverted key and an empty value,
    // don't include the key in the output array
    // since absence from the array denotes this state
    const bool appendKey = ! ( m_KeyInverted && m_Value.IsEmpty() );
    if ( appendKey )
    {
        dirName += m_Key;
        dirNameArray.Append( dirName );
    }

    if ( appendKey && !m_Value.IsEmpty() )
    {
        dirName.Clear();
        // propagate key inverted to value inverted
        // in a double inversion, the inversions cancel each other out
        // so XOR the inversion bools
        const bool valueInverted = ( ( m_KeyInverted ? 1 : 0 ) ^ ( m_ValueInverted ? 1 : 0 ) ) == 1;
        if ( valueInverted )
        {
            dirName += TAG_NOT_OPERATOR_DIR_PREFIX;
        }
        else
        {
            // no inversion
        }
        dirName += m_Value;
        dirNameArray.Append( dirName );
    }
}

// Read (Tag)
//------------------------------------------------------------------------------
void Tag::Read( IOStream & stream )
{
    stream.Read( m_KeyInverted );
    stream.Read( m_Key );
    stream.Read( m_ValueInverted );
    stream.Read( m_Value );
}

// Write (Tag)
//------------------------------------------------------------------------------
void Tag::Write( IOStream & stream ) const
{
    stream.Write( m_KeyInverted );
    stream.Write( m_Key );
    stream.Write( m_ValueInverted );
    stream.Write( m_Value );
}

// operator == (const Tag &) (Tag)
//------------------------------------------------------------------------------
bool Tag::operator == ( const Tag & other ) const
{
    return ( m_KeyInverted == other.m_KeyInverted &&
                 m_ValueInverted == other.m_ValueInverted &&
                 m_Key == other.m_Key &&
                 m_Value == other.m_Value );
}

// operator < (const Tag &) (Tag)
//------------------------------------------------------------------------------
inline bool Tag::operator < ( const Tag & other ) const
{
    return m_Key < other.m_Key;
}

// CONSTRUCTOR (Tags)
//------------------------------------------------------------------------------
Tags::Tags() : m_IsValid( false )
{
}

// DESTRUCTOR (Tags)
//------------------------------------------------------------------------------
Tags::~Tags()
{
    Clear();
}

// GetSize (Tags)
//------------------------------------------------------------------------------
size_t Tags::GetSize() const
{
    return m_Tags.GetSize();
}

// IsValid (Tags)
//------------------------------------------------------------------------------
bool Tags::IsValid() const
{
    return m_IsValid;
}

// IsEmpty (Tags)
//------------------------------------------------------------------------------
bool Tags::IsEmpty() const
{
    return m_Tags.IsEmpty();
}

// SetValid (Tags)
//------------------------------------------------------------------------------
void Tags::SetValid( const bool isValid )
{
    m_IsValid = isValid;
}
        
// Get non-const (Tags)
//------------------------------------------------------------------------------
Tag & Tags::Get( size_t index )
{
    return m_Tags[ index ];
}

// Get const (Tags)
//------------------------------------------------------------------------------
const Tag & Tags::Get( size_t index ) const
{
    return m_Tags[ index ];
}

// Top non-const (Tags)
//------------------------------------------------------------------------------
Tag & Tags::Top()
{
    return m_Tags.Top();
}

// Top const (Tags)
//------------------------------------------------------------------------------
const Tag & Tags::Top() const
{
    return m_Tags.Top();
}

// GetWorkerName (Tags)
//------------------------------------------------------------------------------
const AString & Tags::GetWorkerName() const
{
    return m_WorkerName;
}

// SetWorkerName (Tags)
//------------------------------------------------------------------------------
void Tags::SetWorkerName( const AString & workerName )
{
    m_WorkerName = workerName;
}

// SetCapacity (Tags)
//------------------------------------------------------------------------------
void Tags::SetCapacity( const size_t capacity )
{
    m_Tags.SetCapacity( capacity );
}

// Sort (Tags)
//------------------------------------------------------------------------------
void Tags::Sort()
{
    m_Tags.Sort();
}

// Read (Tags)
//------------------------------------------------------------------------------
void Tags::Read( IOStream & stream )
{
    uint32_t numTags = 0;
    stream.Read( numTags );
    SetCapacity( numTags );
    for ( size_t i=0; i<numTags; ++i )
    {
        Append( Tag() );
        Top().Read( stream );
    }
}

// Write (Tags)
//------------------------------------------------------------------------------
void Tags::Write( IOStream & stream ) const
{
    const uint32_t numTags = (uint32_t)GetSize();
    stream.Write( numTags );
    for ( size_t i=0; i<numTags; ++i )
    {
        Get( i ).Write( stream );
    }
}

// Find (Tags)
//------------------------------------------------------------------------------
bool Tags::FindKey( const char * key, size_t & foundIndex ) const
{
    bool foundKey = false;
    const size_t numExistingTags = GetSize();
    for ( size_t i=0; i<numExistingTags; ++i )
    {
        const Tag & existingTag = Get( i );
        if ( existingTag.GetKey().Equals( key ) )
        {
            foundKey = true;
            foundIndex = i;
            break;
        }
    }
    return foundKey;
}

// MatchesAll (Tags)
//------------------------------------------------------------------------------
bool Tags::MatchesAll( const Tags & searchTags ) const
{
    bool matchesAll = true;  // first assume true
    const size_t numSearchTags = searchTags.GetSize();
    for ( size_t i=0; i<numSearchTags; ++i )
    {
        const Tag & searchTag = searchTags.Get( i );
        const bool searchKeyInverted = searchTag.GetKeyInverted();
        bool matches = searchKeyInverted;  // initialize based on key inverted or not
        const size_t numExistingTags = GetSize();
        for ( size_t j=0; j<numExistingTags; ++j )
        {
            const Tag & existingTag = Get( j );
            if ( searchKeyInverted )
            {
                // match against all existing tags
                matches = matches && existingTag.Matches( searchTag );
                // don't break here, since checking all
            }
            else  // search key not inverted
            {
                matches = existingTag.Matches( searchTag );
                if ( matches )
                {
                    // found a match, so break out of inner loop
                    break;
                }
            }
        }
        if ( !matches )
        {
            matchesAll = false;
            break;
        }
    }
    return matchesAll;
}

// GetNoWorkerFoundError (Tags)
//------------------------------------------------------------------------------
void Tags::GetNoWorkerFoundError(
    const bool distributableJob,
    const AString & nodeName,
    AString & noWorkerError ) const
{
    AStackString<> errorMsg1;
    if ( distributableJob )
    {
        errorMsg1.Format( "Error: No local or remote worker could do the build for '%s'\n",
            nodeName.Get() );
    }
    else
    {
        errorMsg1.Format( "Error in local-only build step: The local worker could not do the build for '%s'\n",
            nodeName.Get() );
    }
    noWorkerError += errorMsg1;
    if ( !m_Tags.IsEmpty() )
    {
        AStackString<> searchTagsArgsString;
        ToArgsString( searchTagsArgsString );
        AStackString<> errorMsg2;
        if ( distributableJob )
        {
            errorMsg2.Format( "Ensure at least one worker has tag(s) that match %s\n", searchTagsArgsString.Get() );
        }
        else
        {
            errorMsg2.Format( "Ensure that the local worker has tag(s) that match %s\n", searchTagsArgsString.Get() );
        }
        noWorkerError += errorMsg2;
        noWorkerError += "Also ensure that the worker(s) can do the build without a system error.\n";
    }
    if ( distributableJob )
    {
        noWorkerError += "Try enabling distributed jobs -dist, ensure workers are available\n";
    }
    if ( !m_Tags.IsEmpty() )
    {
        noWorkerError += "   Local worker: set tags in the LocalWorkerTags of your Settings node\n";
        noWorkerError += "      or pass -Ttag entries to the command line of FBuild\n";
        noWorkerError += "      for sandbox tag, adjust the sandbox fields of your Settings node\n";
        noWorkerError += "      or pass sandbox args to the command line of FBuild\n";
        if ( distributableJob )
        {
            noWorkerError += "   Remote worker: set tags in the WorkerTags of the .settings file\n";
            noWorkerError += "      or pass -Ttag entries to the command line of FBuildWorker\n";
            noWorkerError += "      for sandbox tag, adjust the sandbox fields of the .settings file\n";
            noWorkerError += "      or pass sandbox args to the command line of FBuildWorker\n\n";
        }
    }
}

// ParseAndAddTag (Tags)
//------------------------------------------------------------------------------
void Tags::ParseAndAddTag( const AString & tagString )
{
    bool foundKey = false;
    // try to get the key of the tagString
    const char * keyDelimiter = tagString.Find( TAG_DELIMITER );
    Append( Tag() );
    Tag & newTag = Top();
    if ( keyDelimiter != nullptr )
    {
        if ( keyDelimiter != tagString.Get() )
        {
            // non-empty key
            AStackString<> keyString( tagString.Get(), keyDelimiter );
            newTag.SetKey( keyString );
            AStackString<> valueString( keyDelimiter + 1 );
            newTag.SetValue( valueString );
            foundKey = true;
        }
        else
        {
            // empty key, so add below
        }
    }
    else
    {
        // no delimiter found, so add below
    }

    if ( !foundKey )
    {
        // use whole tag as key and
        // leave value empty
        newTag.SetKey( tagString );
    }
}

// ParseAndAddTags (Tags)
//------------------------------------------------------------------------------
void Tags::ParseAndAddTags( const Array< AString > & tagStrings )
{
    const size_t numTagStrings = tagStrings.GetSize();
    for ( size_t i=0; i<numTagStrings; ++i )
    {
        ParseAndAddTag( tagStrings[ i ] );
    }
}

// Append (Tags)
//------------------------------------------------------------------------------
void Tags::Append( const Tag & tag )
{
    m_Tags.Append( tag );
}

// EraseIndex (Tags)
//------------------------------------------------------------------------------
void Tags::EraseIndex( size_t index )
{
    m_Tags.EraseIndex( index );
}

// Clear (Tags)
//------------------------------------------------------------------------------
void Tags::Clear()
{
    m_IsValid = false;
    m_Tags.Clear();
    m_WorkerName.Clear();
}

// GetChanges (Tags)
//------------------------------------------------------------------------------
void Tags::GetChanges( const Tags & currentTags,
    Tags & removedTags, Tags & addedTags ) const
{
    const size_t numPreviousTags = GetSize();
    const size_t numCurrentTags = currentTags.GetSize();

    // find tags that were removed
    for ( size_t i=0; i<numPreviousTags; ++i )
    {
        const Tag & previousTag = Get( i );
        size_t foundIndex = 0;
        if ( !currentTags.FindKey( previousTag.GetKey().Get(), foundIndex ) )
        {
            // tag no longer present, so mark as removed
            removedTags.Append( previousTag );
        }
    }

    // find tags that were added (or changed)
    for ( size_t i=0; i<numCurrentTags; ++i )
    {
        const Tag & currentTag = currentTags.Get( i );
        size_t foundIndex = 0;
        if ( FindKey( currentTag.GetKey().Get(), foundIndex ) )
        {
            const Tag & previousTag = Get( foundIndex );
            if ( previousTag.GetKeyInverted() != currentTag.GetKeyInverted() ||
                 previousTag.GetValueInverted() != currentTag.GetValueInverted() ||
                 previousTag.GetValue() != currentTag.GetValue() )
            {
                // values differ, so mark as added
                addedTags.Append( currentTag );
            }
            else
            {
                // same key and value; so skip tag,
                // since in previous tags
            }
        }
        else
        {
            // not found, so add it
            addedTags.Append( currentTag );
        }
    }
}

// ApplyChanges (Tags)
//------------------------------------------------------------------------------
bool Tags::ApplyChanges(
    const Tags & removedTags,
    const Tags & addedTags )
{
    bool anyChangesApplied = false;

    // remove tags
    const size_t numRemovedTags = removedTags.GetSize();
    for ( size_t i=0; i<numRemovedTags; ++i )
    {
        const Tag & removedTag = removedTags.Get( i );
        size_t foundIndex = 0;
        if ( FindKey( removedTag.GetKey().Get(), foundIndex ) )
        {
            EraseIndex( foundIndex );
            anyChangesApplied = true;
        }
    }

    // add tags
    const size_t numAddedTags = addedTags.GetSize();
    for ( size_t i=0; i<numAddedTags; ++i )
    {
        const Tag & addedTag = addedTags.Get( i );        
        size_t foundIndex = 0;
        if ( FindKey( addedTag.GetKey().Get(), foundIndex ) )
        {
            Tag & existingTag = Get( foundIndex );
            if ( existingTag.GetKeyInverted() != addedTag.GetKeyInverted() ||
                 existingTag.GetValueInverted() != addedTag.GetValueInverted() ||
                 existingTag.GetValue() != addedTag.GetValue() )
            {
                existingTag = addedTag;
                anyChangesApplied = true;
            }
            else
            {
                // same key and value; so skip adding tag
            }
        }
        else
        {
            Append( addedTag );
            anyChangesApplied = true;
        }
    }

    if ( !m_IsValid )
    {
        // to handle the empty tags case, mark the tags container valid
        // even if no tags were added above
        m_IsValid = true;
        anyChangesApplied = true;
    }

    return anyChangesApplied;
}

// ContainsValidDirChars (Tags)
//------------------------------------------------------------------------------
bool Tags::ContainsValidDirChars( AString & errorMsg ) const
{
    bool containsValidDirChars = true;  // first assume true
    const size_t numExistingTags = GetSize();
    for ( size_t i=0; i<numExistingTags; ++i )
    {
        const Tag & existingTag = Get( i );
        if ( !existingTag.ContainsValidDirChars( errorMsg ) )
        {
            containsValidDirChars = false;
            // don't break here, since want to show user all errors at once
        }
    }
    return containsValidDirChars;
}

// ToStringArray (Tags)
//------------------------------------------------------------------------------
void Tags::ToStringArray( Array< AString > & stringArray ) const
{
    stringArray.Clear();  // clear array, so we can populate it
    const size_t numTags = GetSize();
    for ( size_t i=0; i<numTags; ++i )
    {
        const Tag & workerTag = Get( i );
        AStackString<> tagString;
        workerTag.ToString( tagString );
        stringArray.Append( tagString );
    }
}

// ToArgsString (Tags)
//------------------------------------------------------------------------------
void Tags::ToArgsString( AString & string ) const
{
    const size_t numExistingTags = GetSize();
    for ( size_t i=0; i<numExistingTags; ++i )
    {
        const Tag & existingTag = Get( i );
        if ( i > 0 )
        {
            AStackString<> spaceDelimeter( " " );
            string.Append( spaceDelimeter );
        }
        AStackString<> tagString;
        existingTag.ToString( tagString );
        string.Append( tagString );
    }
}

// operator == (const Tags &) (Tags)
//------------------------------------------------------------------------------
bool Tags::operator == ( const Tags & other ) const
{
    return ( m_WorkerName == other.m_WorkerName &&
                 m_Tags == other.m_Tags );
}

//------------------------------------------------------------------------------
